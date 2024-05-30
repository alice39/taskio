
#ifndef TASKIO_SIMPLE_RUNTIME_GUARD_HEADER
#define TASKIO_SIMPLE_RUNTIME_GUARD_HEADER

#include "runtime.h"

#include "../task/future.h"
#include "../task/task.h"

struct taskio_simple_runtime;

struct taskio_simple_runtime* taskio_simple_runtime_new();
void taskio_simple_runtime_drop(struct taskio_simple_runtime* runtime);

struct taskio_join_handle
taskio_simple_runtime_spawn(struct taskio_simple_runtime* runtime,
                            struct taskio_task* task);

struct taskio_join_handle
taskio_simple_runtime_spawn_blocking(struct taskio_simple_runtime* runtime,
                                     struct taskio_task* task);

void taskio_simple_runtime_run(struct taskio_simple_runtime* runtime);

#endif // TASKIO_SIMPLE_RUNTIME_GUARD_HEADER