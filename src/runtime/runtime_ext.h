#ifndef TASKIO_RUNTIME_EXT_GUARD_HEADER
#define TASKIO_RUNTIME_EXT_GUARD_HEADER

#include "taskio/runtime/runtime.h"

struct taskio_runtime {
    void* inner;

    struct taskio_join_handle (*spawn)(void* inner, struct taskio_task* task);
    struct taskio_join_handle (*spawn_blocking)(void* inner,
                                                struct taskio_task* task);
};

void taskio_context_push_runtime(struct taskio_runtime* runtime);
void taskio_context_pop_runtime();

#endif // TASKIO_RUNTIME_EXT_GUARD_HEADER
