#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include <stdlib.h>

#include "taskio/async.h"
#include "taskio/runtime.h"

#define taskio_main_env __taskio_async_main_env

#define taskio_main_begin() async_fn_begin(int, __taskio_async_main)

#define taskio_main()                                                                                                  \
    static_future_fn(void, __taskio_async_main, int, argc, char**, args) {                                             \
        return_future_fn(void, __taskio_async_main, argc, args);                                                       \
    }                                                                                                                  \
                                                                                                                       \
    int main(int argc, char** args) {                                                                                  \
        taskio_stack_runtime stack_rt = {};                                                                            \
        struct taskio_runtime* rt = (struct taskio_runtime*)stack_rt;                                                  \
        taskio_runtime_init(rt, TASKIO_SINGLE_THREADED);                                                               \
                                                                                                                       \
        if (sizeof(struct __taskio_async_main_future) < 512000) {                                                      \
            struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                \
            taskio_runtime_block_on(rt, &future.inner);                                                                \
        } else {                                                                                                       \
            struct __taskio_async_main_future* future = malloc(sizeof(struct __taskio_async_main_future));             \
            __taskio_async_main_init(future, argc, args);                                                              \
            taskio_runtime_block_on(rt, &future->inner);                                                               \
            free(future);                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        taskio_runtime_drop(rt);                                                                                       \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    async_fn(void, __taskio_async_main)
#endif

#endif // TASKIO_GUARD_HEADER
