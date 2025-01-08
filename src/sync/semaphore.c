#include <taskio/sync/semaphore.h>

static inline void _signal_async(struct taskio_semaphore* semaphore);
static inline void _signal_sync(struct taskio_semaphore* semaphore);

void taskio_semaphore_init_with_alloc(struct taskio_semaphore* semaphore, size_t permits,
                                      struct taskio_allocator* allocator) {
    semaphore->counter = permits;

    semaphore->allocator = *allocator;

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
    struct taskio_allocator allocator = taskio_default_allocator();
    taskio_semaphore_init_with_alloc(semaphore, permits, &allocator);
}

void taskio_semaphore_drop(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    while (node) {
        struct taskio_semaphore_node* next = node->next;
        if (atomic_fetch_sub(&node->counter, 1) == 1) {
            semaphore->allocator.free(semaphore->allocator.data, node);
        }
        node = next;
    }

    cnd_destroy(&semaphore->blocking_wake_cnd);
    mtx_destroy(&semaphore->blocking_wake_mtx);

    mtx_destroy(&semaphore->wake_guard);
}

size_t taskio_semaphore_getvalue(struct taskio_semaphore* semaphore) { return semaphore->counter; }

future_fn_impl(void, taskio_semaphore_wait)(struct taskio_semaphore* semaphore) {
    return_future_fn(void, taskio_semaphore_wait, semaphore);
}

async_fn(void, taskio_semaphore_wait) {
    struct taskio_semaphore* semaphore = async_env(semaphore);

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
                semaphore->allocator.alloc(semaphore->allocator.data, sizeof(struct taskio_semaphore_node));

            mtx_lock(&semaphore->wake_guard);

            node->counter = 2;
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

    async_scope() {
        struct taskio_semaphore_node* node = async_env(node);
        if (atomic_fetch_sub(&node->counter, 1) == 1) {
            semaphore->allocator.free(semaphore->allocator.data, node);
        }

        async_return();
    }
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
        semaphore->allocator.free(semaphore->allocator.data, node);
    }
}

static inline void _signal_sync(struct taskio_semaphore* semaphore) {
    semaphore->blocking_wake_signal += 1;
    cnd_signal(&semaphore->blocking_wake_cnd);
}
