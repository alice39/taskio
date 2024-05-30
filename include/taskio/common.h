#ifndef TASKIO_COMMON_GUARD_HEADER
#define TASKIO_COMMON_GUARD_HEADER

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "async.h"
#include "task/task.h"

struct taskio_join_env {
    struct taskio_task** tasks;
    size_t len;
};

struct taskio_sleep_env {
    struct timespec deadline;
};

#define taskio_join(...)                                                       \
    taskio_join(sizeof((void*[]){__VA_ARGS__}) / sizeof(void*), __VA_ARGS__)

future_fn(void, taskio_join)(size_t len, ...);
future_fn(void, taskio_sleep)(uint64_t seconds, uint64_t nanoseconds);

#endif // TASKIO_COMMON_GUARD_HEADER