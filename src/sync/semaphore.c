#ifdef TASKIO_TRACING
#include <stdio.h>
#endif // TASKIO_TRACING

#include <taskio/sync/semaphore.h>

#include "../alloc_ext.h"

#define SEMAPHORE_NODE_MASK (0b000011)
#define SEMAPHORE_NODE_PLAIN (0b010000)
#define SEMAPHORE_NODE_TIMED (0b100000)

static inline void _signal_async(struct taskio_semaphore* semaphore);
static inline void _signal_sync(struct taskio_semaphore* semaphore);

static inline void _node_drop(struct taskio_allocator* allocator, struct taskio_semaphore_node* node, bool is_parent);

void taskio_semaphore_init_with_alloc(struct taskio_semaphore* semaphore, size_t permits,
                                      struct taskio_allocator* allocator) {
    semaphore->counter = permits;

    semaphore->allocator = allocator ? allocator : taskio_default_allocator_ref();

    mtx_init(&semaphore->blocking_wake_mtx, mtx_plain);
    cnd_init(&semaphore->blocking_wake_cnd);
    semaphore->blocking_wake_wait = 0;
    semaphore->blocking_wake_signal = 0;
    semaphore->blocking_wake_priority = 0;

    mtx_init(&semaphore->wake_guard, mtx_plain);
    semaphore->wake_queue_head = NULL;
    semaphore->wake_queue_tail = NULL;
}

void taskio_semaphore_init(struct taskio_semaphore* semaphore, size_t permits) {
    taskio_semaphore_init_with_alloc(semaphore, permits, NULL);
}

void taskio_semaphore_drop(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    while (node) {
        struct taskio_semaphore_node* next = node->next;
        _node_drop(semaphore->allocator, node, true);
        node = next;
    }

#ifdef TASKIO_TRACING
    if (semaphore->blocking_wake_wait > 0) {
        fprintf(stderr, "taskio-tracing: semaphore dropped before threads wake up on taskio_semaphore_blocking_wait\n");
    }
#endif // TASKIO_TRACING

    cnd_destroy(&semaphore->blocking_wake_cnd);
    mtx_destroy(&semaphore->blocking_wake_mtx);

    mtx_destroy(&semaphore->wake_guard);
}

size_t taskio_semaphore_getvalue(struct taskio_semaphore* semaphore) { return semaphore->counter; }

future_fn_impl(void, taskio_semaphore_wait)(struct taskio_semaphore* semaphore) {
    return_future_fn(void, taskio_semaphore_wait, semaphore);
}

future_fn_impl(bool, taskio_semaphore_timedwait)(struct taskio_semaphore* semaphore, uint64_t delay) {
    struct taskio_semaphore_wait_future wait = taskio_semaphore_wait(semaphore);
    struct taskio_sleep_future timeout = taskio_sleep(delay);

    wait.env.timed = true;
    return_future_fn(bool, taskio_semaphore_timedwait, semaphore, wait, timeout);
}

async_fn(void, taskio_semaphore_wait) {
    struct taskio_semaphore* semaphore = async_env(semaphore);

    async_cleanup() {
        if (async_env(node)) {
            _node_drop(semaphore->allocator, async_env(node), false);
        }
    }

    async_scope() {
        size_t current_value = 0;

        do {
            current_value = semaphore->counter;
            if (current_value == 0) {
                break;
            }
        } while (!atomic_compare_exchange_weak(&semaphore->counter, &current_value, current_value - 1));

        if (current_value == 0) {
            struct taskio_semaphore_node* node = async_env(node) =
                semaphore->allocator->alloc(semaphore->allocator->data, sizeof(struct taskio_semaphore_node));

            mtx_lock(&semaphore->wake_guard);

            node->counter = async_env(timed) ? SEMAPHORE_NODE_TIMED | 2 : SEMAPHORE_NODE_PLAIN | 2;
            node->waker = __TASKIO_FUTURE_CTX->waker;
            node->back = semaphore->wake_queue_tail;
            node->next = NULL;

            if (semaphore->wake_queue_head == NULL) {
                semaphore->wake_queue_head = node;
                semaphore->wake_queue_tail = node;
            } else {
                semaphore->wake_queue_tail->next = node;
                semaphore->wake_queue_tail = node;
            }

            mtx_unlock(&semaphore->wake_guard);

            suspended_yield();
        }

        async_return();
    }

    async_scope() { async_return(); }
}

async_fn(bool, taskio_semaphore_timedwait) {
    async_scope() {
        await_fn_get_as(taskio_select, taskio_select_with, &async_env(result), async_env(semaphore)->allocator, true,
                        NULL, &async_env(wait), &async_env(timeout));
    }

    async_scope() { async_return(async_env(result) == 0); }
}

void taskio_semaphore_blocking_wait(struct taskio_semaphore* semaphore) {
    size_t current_value = 0;

    do {
        current_value = semaphore->counter;
        if (current_value == 0) {
            break;
        }
    } while (!atomic_compare_exchange_weak(&semaphore->counter, &current_value, current_value - 1));

    if (current_value == 0) {
        mtx_lock(&semaphore->blocking_wake_mtx);

        if (semaphore->blocking_wake_signal == 0) {
            semaphore->blocking_wake_wait += 1;
            cnd_wait(&semaphore->blocking_wake_cnd, &semaphore->blocking_wake_mtx);
            semaphore->blocking_wake_wait -= 1;
        }

        semaphore->blocking_wake_signal -= 1;
        mtx_unlock(&semaphore->blocking_wake_mtx);
    }
}

void taskio_semaphore_signal(struct taskio_semaphore* semaphore) {
    mtx_lock(&semaphore->wake_guard);
    mtx_lock(&semaphore->blocking_wake_mtx);

    if (semaphore->wake_queue_head != NULL && semaphore->blocking_wake_wait > 0) {
        size_t priority_value = semaphore->blocking_wake_priority;
        semaphore->blocking_wake_priority += 1;

        if (priority_value % 2 == 0) {
            _signal_async(semaphore);
        } else {
            _signal_sync(semaphore);
        }
    } else if (semaphore->wake_queue_head != NULL) {
        _signal_async(semaphore);
    } else if (semaphore->blocking_wake_wait > 0) {
        _signal_sync(semaphore);
    } else {
        semaphore->counter += 1;
    }

    mtx_unlock(&semaphore->blocking_wake_mtx);
    mtx_unlock(&semaphore->wake_guard);
}

static inline void _signal_async(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    if (node == NULL) {
        return;
    }

    semaphore->wake_queue_head = node->next;
    if (semaphore->wake_queue_head == NULL) {
        semaphore->wake_queue_tail = NULL;
    }

    node->waker.wake(&node->waker);
    if (atomic_fetch_sub(&node->counter, 1) == 1) {
        semaphore->allocator->free(semaphore->allocator->data, node);
    }
}

static inline void _signal_sync(struct taskio_semaphore* semaphore) {
    semaphore->blocking_wake_signal += 1;
    cnd_signal(&semaphore->blocking_wake_cnd);
}

static inline void _node_drop(struct taskio_allocator* allocator, struct taskio_semaphore_node* node, bool is_parent) {
    size_t counter_with_flags = atomic_fetch_sub(&node->counter, 1);
    size_t counter = counter_with_flags & SEMAPHORE_NODE_MASK;

    bool is_timed = (counter_with_flags & SEMAPHORE_NODE_TIMED) != 0;

#ifdef TASKIO_TRACING
    bool is_plain = (counter_with_flags & SEMAPHORE_NODE_PLAIN) != 0;

    if (is_parent && is_plain && counter != 1) {
        fprintf(stderr, "taskio-tracing: semaphore dropped before futures wake up on taskio_semaphore_wait\n");
    }
#endif // TASKIO_TRACING

    if (counter == 1 || (is_parent && !is_timed)) {
        allocator->free(allocator->data, node);
    }
}
