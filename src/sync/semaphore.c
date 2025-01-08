#include <stdlib.h>

#include <taskio/sync/semaphore.h>

void taskio_semaphore_init_with_alloc(struct taskio_semaphore* semaphore, size_t permits,
                                      struct taskio_allocator* allocator) {
    semaphore->counter = permits;

    semaphore->allocator = *allocator;

    mtx_init(&semaphore->wake_guard, mtx_plain);
    semaphore->wake_queue_head = NULL;
    semaphore->wake_queue_tail = NULL;
}

void taskio_semaphore_init(struct taskio_semaphore* semaphore, size_t permits) {
    struct taskio_allocator allocator = {
        .alloc = malloc,
        .free = free,
    };

    taskio_semaphore_init_with_alloc(semaphore, permits, &allocator);
}

void taskio_semaphore_drop(struct taskio_semaphore* semaphore) {
    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    while (node) {
        struct taskio_semaphore_node* next = node->next;
        if (atomic_fetch_sub(&node->counter, 1) == 1) {
            semaphore->allocator.free(node);
        }
        node = next;
    }

    mtx_destroy(&semaphore->wake_guard);
}

size_t taskio_semaphore_getvalue(struct taskio_semaphore* semaphore) { return semaphore->counter; }

future_fn_impl(void, taskio_semaphore_wait)(struct taskio_semaphore* semaphore) {
    return_future_fn(void, taskio_semaphore_wait, semaphore);
}

async_fn(void, taskio_semaphore_wait) {
    async_fn_begin(void, taskio_semaphore_wait);

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
                semaphore->allocator.alloc(sizeof(struct taskio_semaphore_node));

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
            semaphore->allocator.free(node);
        }

        async_return();
    }
}

void taskio_semaphore_signal(struct taskio_semaphore* semaphore) {
    semaphore->counter++;

    mtx_lock(&semaphore->wake_guard);

    struct taskio_semaphore_node* node = semaphore->wake_queue_head;
    if (node) {
        semaphore->wake_queue_head = node->next;
        if (semaphore->wake_queue_head == NULL) {
            semaphore->wake_queue_tail = NULL;
        }
    }

    mtx_unlock(&semaphore->wake_guard);

    if (node) {
        node->waker.wake(&node->waker);
        if (atomic_fetch_sub(&node->counter, 1) == 1) {
            semaphore->allocator.free(node);
        }
    }
}
