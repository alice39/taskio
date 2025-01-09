#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#include <string.h>

#include <taskio/async.h>
#include <taskio/runtime.h>

#define taskio_main_env __taskio_async_main_env

#define taskio_main(...)                                                                                               \
    static_future_fn(int, __taskio_async_main)(int argc, char** args) {                                                \
        return_future_fn(int, __taskio_async_main, argc, args);                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void __taskio_async_main_init(struct __taskio_async_main_future* f, int argc, char** args) {                       \
        f->inner = return_future_fn_inner_obj(void, __taskio_async_main);                                              \
        memset(&f->env, 0, sizeof(struct taskio_main_env));                                                            \
        f->env.argc = argc;                                                                                            \
        f->env.args = args;                                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    int main(int argc, char** args) {                                                                                  \
        struct __taskio_async_main_runtime_config {                                                                    \
            struct taskio_allocator allocator;                                                                         \
            size_t worker_size;                                                                                        \
        };                                                                                                             \
                                                                                                                       \
        struct __taskio_async_main_runtime_config config = {__VA_ARGS__};                                              \
        if (memcmp(&config.allocator, &(struct taskio_allocator){}, sizeof(struct taskio_allocator)) == 0) {           \
            config.allocator = taskio_default_allocator();                                                             \
        }                                                                                                              \
        if (config.worker_size == 0) {                                                                                 \
            config.worker_size = TASKIO_SINGLE_THREADED;                                                               \
        }                                                                                                              \
                                                                                                                       \
        taskio_stack_runtime stack_rt = {};                                                                            \
        struct taskio_runtime* rt = (struct taskio_runtime*)stack_rt;                                                  \
        taskio_runtime_init(rt, config.worker_size, &config.allocator);                                                \
                                                                                                                       \
        int main_result = 0;                                                                                           \
                                                                                                                       \
        if (sizeof(struct __taskio_async_main_future) < 512000) {                                                      \
            struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                \
            taskio_runtime_block_on(rt, &future.inner, &main_result);                                                  \
        } else {                                                                                                       \
            struct __taskio_async_main_future* future =                                                                \
                config.allocator.alloc(config.allocator.data, sizeof(struct __taskio_async_main_future));              \
            __taskio_async_main_init(future, argc, args);                                                              \
            taskio_runtime_block_on(rt, &future->inner, &main_result);                                                 \
            config.allocator.free(config.allocator.data, future);                                                      \
        }                                                                                                              \
                                                                                                                       \
        taskio_runtime_drop(rt);                                                                                       \
        return main_result;                                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    async_fn(void, __taskio_async_main)

#endif // TASKIO_GUARD_HEADER
