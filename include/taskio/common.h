#ifndef TASKIO_COMMON_GUARD_HEADER
#define TASKIO_COMMON_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "async.h"

#define taskio_join(...) taskio_join(sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

struct taskio_sleep_env {
    uint64_t ms;
    struct taskio_future_context ctx;
};

struct taskio_join_task;

struct taskio_join_env {
    size_t len;
    struct taskio_waker waker;

    struct taskio_join_task* poll_head;
    struct taskio_join_task* poll_tail;
};

future_fn(void, taskio_sleep)(uint64_t ms);
future_fn(void, taskio_join)(size_t len, ...);

struct taskio_join_future taskio_join_from_list(size_t len, struct taskio_future** futures);

#endif // TASKIO_COMMON_GUARD_HEADER
