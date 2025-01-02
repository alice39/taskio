#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include "taskio/async.h"
#include "taskio/runtime.h"

#define taskio_main(block, env_var, ...)                                                                               \
    struct __taskio_async_main_env {                                                                                   \
        int argc;                                                                                                      \
        char** args;                                                                                                   \
                                                                                                                       \
        env_var;                                                                                                       \
        MAIN_SCOPE_INIT(__VA_ARGS__)                                                                                   \
    };                                                                                                                 \
    future_fn(void, __taskio_async_main)(int argc, char** args) {                                                      \
        return_future_fn(void, __taskio_async_main, argc, args);                                                       \
    }                                                                                                                  \
    async_fn(                                                                                                          \
        void, __taskio_async_main,                                                                                     \
        {                                                                                                              \
            (void)argc;                                                                                                \
            (void)args;                                                                                                \
            ASYNC_SCOPE_INIT(__VA_ARGS__);                                                                             \
            block                                                                                                      \
        },                                                                                                             \
        async_scope(int, argc), async_scope(char**, args));                                                            \
                                                                                                                       \
    int main(int argc, char** args) {                                                                                  \
        struct taskio_runtime rt;                                                                                      \
        taskio_runtime_init(&rt, TASKIO_SINGLE_THREADED);                                                              \
                                                                                                                       \
        struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                    \
        taskio_runtime_block_on(&rt, (struct taskio_future*)&future);                                                  \
                                                                                                                       \
        taskio_runtime_drop(&rt);                                                                                      \
        return 0;                                                                                                      \
    }                                                                                                                  \
    char __taskio_dummy_fn
#endif

#endif // TASKIO_GUARD_HEADER
