#ifndef TASKIO_RUNTIME_GUARD_HEADEr
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdbool.h>

#include "../task/future.h"
#include "../task/task.h"

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
