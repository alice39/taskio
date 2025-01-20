#ifndef TASKIO_RUNTIME_PLATFORM_GUARD_HEADER
#define TASKIO_RUNTIME_PLATFORM_GUARD_HEADER

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
#include <stdatomic.h>
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

#include <threads.h>

#include <semaphore.h>

#include "../../collections/vec.h"
#include "../../runtime_ext.h"
#include "../../wheel.h"

#define WHEEL_LEVEL_SIZE 8
#define BLOCKING_WORKER_SIZE 512

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

struct taskio_task_base {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    atomic_size_t counter;
#else
    size_t counter;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    enum taskio_task_status status;
    bool blocking;

    struct taskio_runtime* runtime;
    struct taskio_task_wake_node* wake_on_ready_top;

    size_t out_size;
    void* out;
};

struct taskio_task {
    struct taskio_task_base base;

    struct taskio_future* future;
    struct taskio_task* next;

    uint8_t raw_futures[];
};

struct taskio_blocking_task {
    struct taskio_task_base base;

    void (*handler)(void* data, void* out);
    void* data;

    struct taskio_blocking_task* next;

    uint8_t raw_out[];
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
    taskio_vec(taskio_timer) buckets[543];
    struct taskio_wheel_timer wheels[WHEEL_LEVEL_SIZE];
};

struct taskio_worker {
    thrd_t id;
    struct taskio_runtime* runtime;

    struct taskio_task* task; // NULL if not used
    void* task_out;           // NULL if not used
};

struct taskio_blocking_worker {
    thrd_t id;
};

struct taskio_runtime {
    struct taskio_allocator allocator;

    size_t worker_size;
    struct taskio_worker workers[24];

    sem_t blocking_sem;
    struct taskio_blocking_worker blocking_workers[BLOCKING_WORKER_SIZE];

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_t poll_guard;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_t blocking_guard;

    int event_fd;
    int timer_fd;
    int epoll_fd;

    struct taskio_hierarchy_wheel_timer hierarchy_wheel;

    bool poll_scheduled;
    bool blocking_initialized;

    struct taskio_task* poll_head;
    struct taskio_task* poll_tail;

    struct taskio_blocking_task* blocking_head;
    struct taskio_blocking_task* blocking_tail;
};

#endif // TASKIO_RUNTIME_PLATFORM_GUARD_HEADER
