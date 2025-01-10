#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#include <stdlib.h>
#include <string.h>

#include <taskio/async.h>
#include <taskio/runtime.h>

#define taskio_main_env __taskio_async_main_env

#define taskio_main(...)                                                                                               \
    static void* _taskio_alloc([[maybe_unused]] void* data, size_t bytes) { return malloc(bytes); }                    \
                                                                                                                       \
    static void _taskio_free([[maybe_unused]] void* data, void* ptr) { free(ptr); }                                    \
                                                                                                                       \
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
            config.allocator.alloc = _taskio_alloc;                                                                    \
            config.allocator.free = _taskio_free;                                                                      \
            config.allocator.data = NULL;                                                                              \
        }                                                                                                              \
        if (config.worker_size == 0) {                                                                                 \
            config.worker_size = TASKIO_SINGLE_THREADED;                                                               \
        }                                                                                                              \
                                                                                                                       \
        taskio_initialize_allocator(&config.allocator);                                                                \
        struct taskio_allocator* allocator = taskio_default_allocator();                                               \
                                                                                                                       \
        taskio_stack_runtime stack_rt = {};                                                                            \
        struct taskio_runtime* rt = (struct taskio_runtime*)stack_rt;                                                  \
        taskio_runtime_init(rt, config.worker_size, allocator);                                                        \
                                                                                                                       \
        int main_result = 0;                                                                                           \
                                                                                                                       \
        if (sizeof(struct __taskio_async_main_future) < 512000) {                                                      \
            struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                \
            taskio_runtime_block_on(rt, &future.inner, &main_result);                                                  \
        } else {                                                                                                       \
            struct __taskio_async_main_future* future =                                                                \
                allocator->alloc(allocator->data, sizeof(struct __taskio_async_main_future));                          \
            __taskio_async_main_init(future, argc, args);                                                              \
            taskio_runtime_block_on(rt, &future->inner, &main_result);                                                 \
            allocator->free(allocator->data, future);                                                                  \
        }                                                                                                              \
                                                                                                                       \
        taskio_runtime_drop(rt);                                                                                       \
        return main_result;                                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    async_fn(void, __taskio_async_main)

#endif // TASKIO_GUARD_HEADER
