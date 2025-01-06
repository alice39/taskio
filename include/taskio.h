#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include "taskio/async.h"
#include "taskio/runtime.h"

#define MAIN_ENV_DECL(...) __VA_OPT__(MAIN_ENV_DECL_1(__VA_ARGS__))

#define MAIN_ENV_DECL_1(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_2(__VA_ARGS__))
#define MAIN_ENV_DECL_2(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_3(__VA_ARGS__))
#define MAIN_ENV_DECL_3(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_4(__VA_ARGS__))
#define MAIN_ENV_DECL_4(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_5(__VA_ARGS__))
#define MAIN_ENV_DECL_5(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_6(__VA_ARGS__))
#define MAIN_ENV_DECL_6(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_7(__VA_ARGS__))
#define MAIN_ENV_DECL_7(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_8(__VA_ARGS__))
#define MAIN_ENV_DECL_8(T, name, ...)                                                                                  \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_ENV_DECL_9(__VA_ARGS__))
#define MAIN_ENV_DECL_9(T, name, ...) T name;

#define taskio_main_begin() async_fn_begin(int, __taskio_async_main)

#define taskio_main(env_var, ...)                                                                                      \
    struct __taskio_async_main_env {                                                                                   \
        int argc;                                                                                                      \
        char** args;                                                                                                   \
                                                                                                                       \
        env_var;                                                                                                       \
        MAIN_ENV_DECL(__VA_ARGS__)                                                                                     \
    };                                                                                                                 \
                                                                                                                       \
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
