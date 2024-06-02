#ifndef TASKIO_RUNTIME_GUARD_HEADER
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdbool.h>

#include "../async.h"
#include "../task/future.h"
#include "../task/task.h"

#define taskio_task_spawn_with_handle(handle, future)                          \
    {                                                                          \
        handle = taskio_runtime_spawn(taskio_task_new(future));                \
        taskio_defer_fn(taskio_join_handle_drop, &__future_handle);            \
    }

#define taskio_task_spawn_fn_with_handle(handle, fn, ...)                      \
    {                                                                          \
        struct fn##_future __future_obj_to_spawn = fn(__VA_ARGS__);            \
        handle = taskio_runtime_spawn(taskio_task_new(__future_obj_to_spawn)); \
        taskio_defer_fn(taskio_join_handle_drop, &handle);                     \
    }

#define taskio_task_spawn(future)                                              \
    {                                                                          \
        struct taskio_join_handle __future_handle =                            \
            taskio_runtime_spawn(taskio_task_new(future));                     \
        taskio_join_handle_drop(&__future_handle);                             \
    }

#define taskio_task_spawn_fn(fn, ...)                                          \
    {                                                                          \
        struct fn##_future __future_obj_to_spawn = fn(__VA_ARGS__);            \
        struct taskio_join_handle __future_handle =                            \
            taskio_runtime_spawn(taskio_task_new(__future_obj_to_spawn));      \
        taskio_join_handle_drop(&__future_handle);                             \
    }

struct taskio_join_handle {
    void (*abort)(struct taskio_join_handle*);

    bool (*is_aborted)(struct taskio_join_handle*);
    bool (*is_finished)(struct taskio_join_handle*);
    // check if future has been destroyed by runtime
    bool (*is_destroyed)(struct taskio_join_handle*);

    void* task;
};

struct taskio_join_handle taskio_runtime_spawn(struct taskio_task* task);
struct taskio_join_handle
taskio_runtime_spawn_blocking(struct taskio_task* task);

struct taskio_join_handle
taskio_join_handle_clone(struct taskio_join_handle* handle);
void taskio_join_handle_drop(struct taskio_join_handle* handle);

#endif // TASKIO_RUNTIME_GUARD_HEADER
