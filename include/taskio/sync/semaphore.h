#ifndef TASKIO_SYNC_SEMAPHORE_GUARD_HEADER
#define TASKIO_SYNC_SEMAPHORE_GUARD_HEADER

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
#include <stdatomic.h>
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

#include <stdint.h>
#include <threads.h>

#include <taskio/alloc.h>
#include <taskio/async.h>
#include <taskio/common.h>

struct taskio_semaphore_node {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    atomic_size_t counter;
#else
    size_t counter;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    struct taskio_waker waker;

    struct taskio_semaphore_node* back;
    struct taskio_semaphore_node* next;
};

struct taskio_semaphore {
    struct taskio_allocator* allocator;

    mtx_t mtx_guard;
    cnd_t cnd_guard;

    size_t counter;
    size_t priority;

    size_t blocking_wake_wait;

    struct taskio_semaphore_node* wake_queue_head;
    struct taskio_semaphore_node* wake_queue_tail;
};

void taskio_semaphore_init(struct taskio_semaphore* semaphore, size_t permits);
void taskio_semaphore_init_with_alloc(struct taskio_semaphore* semaphore, struct taskio_allocator* allocator,
                                      size_t permits);
void taskio_semaphore_drop(struct taskio_semaphore* semaphore);

size_t taskio_semaphore_getvalue(struct taskio_semaphore* semaphore);

struct taskio_semaphore_wait_env {
    struct taskio_semaphore* semaphore;
    struct taskio_semaphore_node* node;

    bool timed;
};

future_fn(void, taskio_semaphore_wait)(struct taskio_semaphore* semaphore);

struct taskio_semaphore_timedwait_env {
    struct taskio_semaphore* semaphore;

    struct taskio_semaphore_wait_future wait;
    struct taskio_sleep_future timeout;

    size_t result;

    future_env(taskio_select);
};

future_fn(bool, taskio_semaphore_timedwait)(struct taskio_semaphore* semaphore, uint64_t delay);

void taskio_semaphore_blocking_wait(struct taskio_semaphore* semaphore);
void taskio_semaphore_signal(struct taskio_semaphore* semaphore);

#endif // TASKIO_SYNC_SEMAPHORE_GUARD_HEADER
