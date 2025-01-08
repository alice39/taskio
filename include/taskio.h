#ifndef TASKIO_GUARD_HEADER
#define TASKIO_GUARD_HEADER

#if defined(TASKIO_RUNTIME) && TASKIO_RUNTIME == SIMPLE

#include <stdlib.h>
#include <string.h>

#include "taskio/async.h"
#include "taskio/runtime.h"

#define __TASKIO_MAIN_CUSTOM_ALLOC(allocator, alloc_fn, free_fn)                                                       \
    allocator.alloc = alloc_fn;                                                                                        \
    allocator.free = free_fn;

#define taskio_main_env __taskio_async_main_env

#define taskio_main_begin() async_fn_begin(int, __taskio_async_main)

#define taskio_main(...)                                                                                               \
    static_future_fn(void, __taskio_async_main)(int argc, char** args) {                                               \
        return_future_fn(void, __taskio_async_main, argc, args);                                                       \
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
        taskio_stack_runtime stack_rt = {};                                                                            \
        struct taskio_runtime* rt = (struct taskio_runtime*)stack_rt;                                                  \
        struct taskio_allocator allocator = {.alloc = malloc, .free = free};                                           \
        __VA_OPT__(__TASKIO_MAIN_CUSTOM_ALLOC(allocator, __VA_ARGS__))                                                 \
        taskio_runtime_init(rt, TASKIO_SINGLE_THREADED, &allocator);                                                   \
                                                                                                                       \
        if (sizeof(struct __taskio_async_main_future) < 512000) {                                                      \
            struct __taskio_async_main_future future = __taskio_async_main(argc, args);                                \
            taskio_runtime_block_on(rt, &future.inner);                                                                \
        } else {                                                                                                       \
            struct __taskio_async_main_future* future = allocator.alloc(sizeof(struct __taskio_async_main_future));    \
            __taskio_async_main_init(future, argc, args);                                                              \
            taskio_runtime_block_on(rt, &future->inner);                                                               \
            allocator.free(future);                                                                                    \
        }                                                                                                              \
                                                                                                                       \
        taskio_runtime_drop(rt);                                                                                       \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    async_fn(void, __taskio_async_main)
#endif

#endif // TASKIO_GUARD_HEADER
