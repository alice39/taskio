#ifndef TASKIO_RUNTIME_PLATFORM_GUARD_HEADER
#define TASKIO_RUNTIME_PLATFORM_GUARD_HEADER

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
#include <stdatomic.h>
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

#include <threads.h>

#include "../../runtime_ext.h"
#include "../../wheel.h"

#define WHEEL_LEVEL_SIZE 8

struct taskio_task_wake_node {
    struct taskio_waker waker;
    void* out;

    struct taskio_task_wake_node* next;
};

enum taskio_task_status {
    taskio_task_suspended,
    taskio_task_scheduled,
    taskio_task_aborted,
    taskio_task_finished,
};

struct taskio_task_out {
    void* out;
    struct taskio_task_out* next;
};

struct taskio_task {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    atomic_size_t counter;
#else
    size_t counter;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    enum taskio_task_status status;

    struct taskio_runtime* runtime;
    struct taskio_task_wake_node* wake_on_ready_top;

    size_t out_size;
    void* out;

    struct taskio_future* future;
    struct taskio_task* next;

    uint8_t raw_futures[];
};

struct taskio_hierarchy_wheel_timer {
// count how many timers there are in total
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    atomic_size_t timer_len;
#else
    size_t timer_len;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    // bucket size is measured by each wheel len:
    // 10 + 10 + 10 + 60 + 60 + 24 + 365 + 4
    struct taskio_timer* buckets[543];
    struct taskio_wheel_timer wheels[WHEEL_LEVEL_SIZE];
};

struct taskio_worker {
    thrd_t id;
    struct taskio_runtime* runtime;

    struct taskio_task* task; // NULL if not used
    void* task_out;           // NULL if not used
};

struct taskio_runtime {
    struct taskio_allocator allocator;

    size_t worker_size;
    struct taskio_worker workers[24];

    int event_fd;
    int timer_fd;
    int epoll_fd;

    struct taskio_hierarchy_wheel_timer hierarchy_wheel;

    bool poll_scheduled;
    struct taskio_task* poll_head;
    struct taskio_task* poll_tail;
};

#endif // TASKIO_RUNTIME_PLATFORM_GUARD_HEADER
