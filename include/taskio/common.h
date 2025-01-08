#ifndef TASKIO_COMMON_GUARD_HEADER
#define TASKIO_COMMON_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "async.h"

#define taskio_join(...)                                                                                               \
    taskio_join(                                                                                                       \
        &(struct taskio_allocator){                                                                                    \
            .alloc = taskio_default_alloc,                                                                             \
            .free = taskio_default_free,                                                                               \
            .data = taskio_default_data(),                                                                             \
        },                                                                                                             \
        sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

#define taskio_join_with_alloc(alloc, ...)                                                                             \
    taskio_join(alloc, sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

struct taskio_sleep_env {
    uint64_t ms;
    struct taskio_waker waker;

    void* timer;
};

struct taskio_join_task;

struct taskio_join_env {
    struct taskio_allocator allocator;

    size_t len;
    size_t completed_len;

    struct taskio_waker waker;

    struct taskio_join_task* head;

    struct taskio_join_task* poll_head;
    struct taskio_join_task* poll_tail;
};

future_fn(void, taskio_sleep)(uint64_t ms);
heap_future_fn(void, taskio_join)(struct taskio_allocator* allocator, size_t len, ...);

struct taskio_join_future taskio_join_from_list(struct taskio_allocator* allocator, size_t len,
                                                struct taskio_future** futures);

#endif // TASKIO_COMMON_GUARD_HEADER
