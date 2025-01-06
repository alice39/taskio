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

struct taskio_join_env {
    size_t len;
    bool is_stack;

    union {
        struct taskio_future* stack_futures[16];
        struct taskio_future** heap_futures;
    };
};

struct taskio_join_list_env {
    size_t len;
    struct taskio_future** futures;
};

future_fn(void, taskio_sleep)(uint64_t ms);
future_fn(void, taskio_join)(size_t len, ...);
future_fn(void, taskio_join_list)(size_t len, struct taskio_future** futures);

#endif // TASKIO_COMMON_GUARD_HEADER
