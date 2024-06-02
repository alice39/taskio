#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include "taskio/async.h"
#include "taskio/runtime/simple_runtime.h"

#define taskio_main(block)                                                     \
    async_fn(void, __taskio_async_main, block);                                \
                                                                               \
    int main() {                                                               \
        struct taskio_simple_runtime* rt = taskio_simple_runtime_new();        \
                                                                               \
        struct __taskio_async_main_future future = __taskio_async_main();      \
        struct taskio_join_handle handle =                                     \
            taskio_simple_runtime_spawn(rt, taskio_task_new(future));          \
        taskio_join_handle_drop(&handle);                                      \
                                                                               \
        taskio_simple_runtime_run(rt);                                         \
        taskio_simple_runtime_drop(rt);                                        \
        return 0;                                                              \
    }                                                                          \
    char __taskio_dummy_fn
#endif

#endif // TASKIO_GUARD_HEADER