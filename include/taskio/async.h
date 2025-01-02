#ifndef TASKIO_ASYNC_GUARD_HEADER
#define TASKIO_ASYNC_GUARD_HEADER

#include <setjmp.h>

#include "future.h"

#define __TASKIO_FUTURE_OBJ_ANY __taskio_future
#define __TASKIO_FUTURE_OBJ __taskio_future_own
#define __TASKIO_FUTURE_CTX __taskio_ctx
#define __TASKIO_FUTURE_POL __taskio_poll
#define __TASKIO_FUTURE_VAL_ANY __taskio_val
#define __TASKIO_FUTURE_VAL __taskio_val_own

#define __TASKIO_TASK_CTX __taskio_task_ctx
#define __TASKIO_TASK_POL __taskio_task_poll

#define future_env(...) __VA_OPT__(union {future_env_1(__VA_ARGS__)})

#define future_env_1(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_2(__VA_ARGS__))
#define future_env_2(name, ...) struct name##_future name;

#define FUTURE_ENV_INIT(...) __VA_OPT__(FUTURE_ENV_INIT_1(__VA_ARGS__))

#define FUTURE_ENV_INIT_1(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_2(__VA_ARGS__))
#define FUTURE_ENV_INIT_2(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_3(__VA_ARGS__))
#define FUTURE_ENV_INIT_3(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_4(__VA_ARGS__))
#define FUTURE_ENV_INIT_4(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_5(__VA_ARGS__))
#define FUTURE_ENV_INIT_5(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_6(__VA_ARGS__))
#define FUTURE_ENV_INIT_6(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_7(__VA_ARGS__))
#define FUTURE_ENV_INIT_7(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_8(__VA_ARGS__))
#define FUTURE_ENV_INIT_8(first, ...) .first = first, __VA_OPT__(FUTURE_ENV_INIT_9(__VA_ARGS__))
#define FUTURE_ENV_INIT_9(first, ...) .first = first,

#define MAIN_SCOPE_INIT(...) __VA_OPT__(MAIN_SCOPE_INIT_1(__VA_ARGS__))

#define MAIN_SCOPE_INIT_1(T, name, ...)                                                                                \
    T name;                                                                                                            \
    __VA_OPT__(MAIN_SCOPE_INIT_2(__VA_ARGS__))
#define MAIN_SCOPE_INIT_2(T, name, ...) T name;

#define ASYNC_SCOPE_INIT(...) __VA_OPT__(ASYNC_SCOPE_INIT_1(__VA_ARGS__))

#define ASYNC_SCOPE_INIT_1(type, name, ...)                                                                            \
    type name = __TASKIO_FUTURE_OBJ->env.name;                                                                         \
    __VA_OPT__(ASYNC_SCOPE_INIT_2(__VA_ARGS__))
#define ASYNC_SCOPE_INIT_2(type, name, ...) type name = __TASKIO_FUTURE_OBJ->env.name;

#define async_scope(T, name) T, name

#define future_fn(T, name, ...)                                                                                        \
    struct name##_future {                                                                                             \
        struct taskio_future inner;                                                                                    \
                                                                                                                       \
        struct name##_env env;                                                                                         \
    };                                                                                                                 \
                                                                                                                       \
    void name##_poll(struct taskio_future* __TASKIO_FUTURE_OBJ, struct taskio_future_context* __TASKIO_FUTURE_CTX,     \
                     enum taskio_future_poll* __TASKIO_FUTURE_POL, void* __TASKIO_FUTURE_VAL);                         \
                                                                                                                       \
    struct name##_future(name)

#define return_future_fn(T, name, ...)                                                                                 \
    return (struct name##_future) {                                                                                    \
        .inner = {.poll = name##_poll}, .env = (struct name##_env){FUTURE_ENV_INIT(__VA_ARGS__)},                      \
    }

#define async_fn(T, name, block, ...)                                                                                  \
    void name##_poll(struct taskio_future* __TASKIO_FUTURE_OBJ_ANY, struct taskio_future_context* __TASKIO_FUTURE_CTX, \
                     enum taskio_future_poll* __TASKIO_FUTURE_POL, void* __TASKIO_FUTURE_VAL_ANY) {                    \
        struct name##_future* __TASKIO_FUTURE_OBJ = (struct name##_future*)__TASKIO_FUTURE_OBJ_ANY;                    \
        T* __TASKIO_FUTURE_VAL = __TASKIO_FUTURE_VAL_ANY;                                                              \
        (void)__TASKIO_FUTURE_OBJ;                                                                                     \
        (void)__TASKIO_FUTURE_CTX;                                                                                     \
        (void)__TASKIO_FUTURE_VAL;                                                                                     \
        ASYNC_SCOPE_INIT(__VA_ARGS__);                                                                                 \
        if (__TASKIO_FUTURE_CTX->waker.can_jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth]) {                                \
            __TASKIO_FUTURE_CTX->waker.can_jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth] = false;                          \
            longjmp(__TASKIO_FUTURE_CTX->waker.jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth], 1);                          \
        }                                                                                                              \
        block;                                                                                                         \
        *__TASKIO_FUTURE_POL = TASKIO_FUTURE_READY;                                                                    \
    }                                                                                                                  \
    char __taskio_dummy_fn

#define yield()                                                                                                        \
    {                                                                                                                  \
        *__TASKIO_FUTURE_POL = TASKIO_FUTURE_PENDING;                                                                  \
        if (setjmp(__TASKIO_FUTURE_CTX->waker.jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth]) == 0) {                       \
            __TASKIO_FUTURE_CTX->waker.wake(&__TASKIO_FUTURE_CTX->waker);                                              \
            return;                                                                                                    \
        }                                                                                                              \
    }

#define await_get(future, out)                                                                                         \
    {                                                                                                                  \
        while (1) {                                                                                                    \
            struct taskio_future_context __TASKIO_TASK_CTX = *__TASKIO_FUTURE_CTX;                                     \
            __TASKIO_TASK_CTX.waker.jmp_depth++;                                                                       \
            enum taskio_future_poll __TASKIO_TASK_POL = TASKIO_FUTURE_PENDING;                                         \
            future.inner.poll(&future.inner, &__TASKIO_TASK_CTX, &__TASKIO_TASK_POL, out);                             \
                                                                                                                       \
            if (__TASKIO_TASK_POL == TASKIO_FUTURE_PENDING) {                                                          \
                if (setjmp(__TASKIO_FUTURE_CTX->waker.jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth]) == 0) {               \
                    __TASKIO_FUTURE_CTX->waker.can_jmp[__TASKIO_FUTURE_CTX->waker.jmp_depth] = true;                   \
                    return;                                                                                            \
                }                                                                                                      \
            } else if (__TASKIO_TASK_POL == TASKIO_FUTURE_READY) {                                                     \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }

#define await_fn_get(fn, out, ...)                                                                                     \
    {                                                                                                                  \
        __TASKIO_FUTURE_OBJ->env.fn = fn(__VA_ARGS__);                                                                 \
        await_get(__TASKIO_FUTURE_OBJ->env.fn, out);                                                                   \
    }

#define await(future) await_get(future, NULL)

#define await_fn(fn, ...) await_fn_get(fn, NULL, __VA_ARGS__)

#define async_return(value)                                                                                            \
    *__TASKIO_FUTURE_POL = TASKIO_FUTURE_READY;                                                                        \
    if (__TASKIO_FUTURE_VAL) {                                                                                         \
        *__TASKIO_FUTURE_VAL = value;                                                                                  \
    }                                                                                                                  \
    return;

#endif // TASKIO_ASYNC_GUARD_HEADER
