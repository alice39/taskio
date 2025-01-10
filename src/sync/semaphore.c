#ifdef TASKIO_TRACING_FEATURE
#include <stdio.h>
#endif // TASKIO_TRACING_FEATURE

#include <taskio/alloc.h>
#include <taskio/sync/semaphore.h>

#define SEMAPHORE_NODE_MASK (0b000011)
#define SEMAPHORE_NODE_PLAIN (0b010000)
#define SEMAPHORE_NODE_TIMED (0b100000)

static inline void _signal_async(struct taskio_semaphore* semaphore);
static inline void _signal_sync(struct taskio_semaphore* semaphore);

static inline void _node_drop(struct taskio_allocator* allocator, struct taskio_semaphore_node* node, bool will_wake);

void taskio_semaphore_init_with_alloc(struct taskio_semaphore* semaphore, struct taskio_allocator* allocator,
                                      size_t permits) {
    semaphore->allocator = allocator ? allocator : taskio_default_allocator();

    mtx_init(&semaphore->mtx_guard, mtx_plain);
    cnd_init(&semaphore->cnd_guard);

    semaphore->counter = permits;
    semaphore->priority = 0;

    semaphore->blocking_wake_wait = 0;

    semaphore->wake_queue_head = NULL;
    semaphore->wake_queue_tail = NULL;
}

void taskio_semaphore_init(struct taskio_semaphore* semaphore, size_t permits) {
    taskio_semaphore_init_with_alloc(semaphore, NULL, permits);
}

void taskio_semaphore_drop(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    while (node) {
        struct taskio_semaphore_node* next = node->next;
        _node_drop(semaphore->allocator, node, false);
        node = next;
    }

#ifdef TASKIO_TRACING_FEATURE
    if (semaphore->blocking_wake_wait > 0) {
        fprintf(stderr, "taskio-tracing: semaphore dropped before threads wake up on taskio_semaphore_blocking_wait\n");
    }
#endif // TASKIO_TRACING_FEATURE

    cnd_destroy(&semaphore->cnd_guard);
    mtx_destroy(&semaphore->mtx_guard);
}

size_t taskio_semaphore_getvalue(struct taskio_semaphore* semaphore) {
    mtx_lock(&semaphore->mtx_guard);
    size_t counter = semaphore->counter;
    mtx_unlock(&semaphore->mtx_guard);

    return counter;
}

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
    struct taskio_semaphore_node* node = async_env(node);

    async_cleanup() {
        if (node) {
            _node_drop(semaphore->allocator, node, true);
        }
    }

    async_scope() {
        mtx_lock(&semaphore->mtx_guard);

        if (semaphore->counter > 0) {
            semaphore->counter -= 1;
        } else {
            struct taskio_allocator* allocator = semaphore->allocator;
            node = async_env(node) = allocator->alloc(allocator->data, sizeof(struct taskio_semaphore_node));

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
        }

        mtx_unlock(&semaphore->mtx_guard);

        if (node) {
            suspended_yield();
        } else {
            async_return();
        }
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
    mtx_lock(&semaphore->mtx_guard);

    if (semaphore->counter > 0) {
        semaphore->counter -= 1;
    } else {
        semaphore->blocking_wake_wait += 1;
        cnd_wait(&semaphore->cnd_guard, &semaphore->mtx_guard);
        semaphore->blocking_wake_wait -= 1;
    }

    mtx_unlock(&semaphore->mtx_guard);
}

void taskio_semaphore_signal(struct taskio_semaphore* semaphore) {
    mtx_lock(&semaphore->mtx_guard);

    if (semaphore->wake_queue_head != NULL && semaphore->blocking_wake_wait > 0) {
        size_t priority_value = semaphore->priority++;

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

    mtx_unlock(&semaphore->mtx_guard);
}

static inline void _signal_async(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;

    semaphore->wake_queue_head = node->next;
    if (semaphore->wake_queue_head == NULL) {
        semaphore->wake_queue_tail = NULL;
    }

    node->waker.wake(&node->waker);
    _node_drop(semaphore->allocator, node, true);
}

static inline void _signal_sync(struct taskio_semaphore* semaphore) { cnd_signal(&semaphore->cnd_guard); }

static inline void _node_drop(struct taskio_allocator* allocator, struct taskio_semaphore_node* node, bool will_wake) {
    size_t counter_with_flags = atomic_fetch_sub(&node->counter, 1);
    size_t counter = counter_with_flags & SEMAPHORE_NODE_MASK;

    bool is_timed = (counter_with_flags & SEMAPHORE_NODE_TIMED) != 0;

#ifdef TASKIO_TRACING_FEATURE
    bool is_plain = (counter_with_flags & SEMAPHORE_NODE_PLAIN) != 0;

    if (!will_wake && is_plain && counter != 1) {
        fprintf(stderr, "taskio-tracing: semaphore dropped before futures wake up on taskio_semaphore_wait\n");
    }
#endif // TASKIO_TRACING_FEATURE

    if (counter == 1 || !(will_wake || is_timed)) {
        allocator->free(allocator->data, node);
    }
}
