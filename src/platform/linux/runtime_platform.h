#ifndef TASKIO_RUNTIME_PLATFORM_GUARD_HEADER
#define TASKIO_RUNTIME_PLATFORM_GUARD_HEADER

#include "../../runtime_ext.h"
#include "../../wheel.h"

struct taskio_task_wake_node {
    struct taskio_waker waker;
    void* out;

    struct taskio_task_wake_node* next;
};

enum taskio_task_status {
    taskio_task_scheduled,
    taskio_task_aborted,
    taskio_task_finished,
};

struct taskio_task {
    atomic_size_t counter;
    uint64_t id;

    bool awaken;
    enum taskio_task_status status;

    struct taskio_runtime* runtime;
    struct taskio_task_wake_node* wake_on_ready_top;

    struct taskio_future* future;
    struct taskio_task* next;

    uint8_t raw_futures[];
};

struct taskio_hierarchy_wheel_timer {
    // bucket size is measured by each wheel len:
    // 10 + 10 + 10 + 60 + 60 + 24 + 365 + 4
    struct taskio_timer* buckets[543];
    struct taskio_wheel_timer wheels[8];
};

struct taskio_worker {
    thrd_t id;
    struct taskio_runtime* runtime;

    uint64_t handle_id; // -1 if undefined
    void* handle_out;   // NULL if undefined
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
