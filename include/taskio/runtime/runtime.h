#ifndef TASKIO_RUNTIME_GUARD_HEADER
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdbool.h>

#include "../task/future.h"
#include "../task/task.h"

#define taskio_task_spawn(future) taskio_runtime_spawn(taskio_task_new(future))
#define taskio_task_spawn_fn(fn, ...)                                          \
    {                                                                          \
        struct fn##_future __future_obj_to_spawn = fn(__VA_ARGS__);            \
        taskio_runtime_spawn(taskio_task_new(__future_obj_to_spawn));          \
    }

struct taskio_join_handle {
    void (*await)(struct taskio_task*, void*);
    void (*abort)(struct taskio_task*);

    bool (*is_finished)(struct taskio_task*);
    bool (*is_aborted)(struct taskio_task*);

    struct taskio_task* task;
};

struct taskio_join_handle taskio_runtime_spawn(struct taskio_task* task);
struct taskio_join_handle
taskio_runtime_spawn_blocking(struct taskio_task* task);

#endif // TASKIO_RUNTIME_GUARD_HEADER
