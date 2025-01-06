#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include "taskio/async.h"
#include "taskio/runtime.h"

#define taskio_main_env __taskio_async_main_env

#define taskio_main_begin() async_fn_begin(int, __taskio_async_main)

#define taskio_main()                                                                                                  \
    future_fn(void, __taskio_async_main)(int argc, char** args) {                                                      \
        return_future_fn(void, __taskio_async_main, argc, args);                                                       \
    }                                                                                                                  \
                                                                                                                       \
    int main(int argc, char** args) {                                                                                  \
        struct taskio_runtime rt;                                                                                      \
        taskio_runtime_init(&rt, TASKIO_SINGLE_THREADED);                                                              \
                                                                                                                       \
        struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                    \
        taskio_runtime_block_on(&rt, &future.inner);                                                                   \
                                                                                                                       \
        taskio_runtime_drop(&rt);                                                                                      \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    async_fn(void, __taskio_async_main)
#endif

#endif // TASKIO_GUARD_HEADER
