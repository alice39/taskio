#ifndef TASKIO_ASYNC_GUARD_HEADER
#define TASKIO_ASYNC_GUARD_HEADER

#include <stdbool.h>
#include <ucontext.h>

#include "task/future.h"
#include "task/task.h"

#define __TASKIO_FUTURE_OBJ __taskio_future
#define __TASKIO_FUTURE_CTX __taskio_ctx
#define __TASKIO_FUTURE_POL __taskio_poll
#define __TASKIO_FUTURE_VAL __taskio_val

#define __TASKIO_TASK_OBJ __taskio_task
#define __TASKIO_TASK_POL __taskio_task_poll

#define future_fn(T, name)                                                     \
    struct name##_future {                                                     \
        void (*poll)(struct name##_future*, struct taskio_future_context*,     \
                     enum taskio_future_poll*, T*);                            \
        void (*drop)(struct name##_future*);                                   \
                                                                               \
        ucontext_t* exec_ucp;                                                  \
        ucontext_t poll_ucp;                                                   \
        ucontext_t drop_ucp;                                                   \
                                                                               \
        struct name##_env env;                                                 \
    };                                                                         \
                                                                               \
    struct name##_future(name)

#define async_fn(T, name, block, ...)                                          \
    struct name##_env {                                                        \
        bool __polling;                                                        \
        bool __dropped;                                                        \
    };                                                                         \
                                                                               \
    future_fn(T, name)(__VA_ARGS__);                                           \
                                                                               \
    void name##_poll(struct name##_future* __TASKIO_FUTURE_OBJ,                \
                     struct taskio_future_context* __TASKIO_FUTURE_CTX,        \
                     enum taskio_future_poll* __TASKIO_FUTURE_POL,             \
                     T* __TASKIO_FUTURE_VAL) {                                 \
        (void)__TASKIO_FUTURE_OBJ;                                             \
        (void)__TASKIO_FUTURE_CTX;                                             \
        (void)__TASKIO_FUTURE_VAL;                                             \
        block;                                                                 \
        *__TASKIO_FUTURE_POL = TASKIO_FUTURE_READY;                            \
    }                                                                          \
                                                                               \
    void name##_drop(struct name##_future* __TASKIO_FUTURE_OBJ) {              \
        __TASKIO_FUTURE_OBJ->env.__dropped = true;                             \
        if (__TASKIO_FUTURE_OBJ->env.__polling) {                              \
            swapcontext(&__TASKIO_FUTURE_OBJ->drop_ucp,                        \
                        &__TASKIO_FUTURE_OBJ->poll_ucp);                       \
        }                                                                      \
    }                                                                          \
                                                                               \
    struct name##_future(name)(__VA_ARGS__) {                                  \
        return (struct name##_future){                                         \
            .poll = name##_poll,                                               \
            .drop = name##_drop,                                               \
            .env =                                                             \
                {                                                              \
                    .__polling = false,                                        \
                    .__dropped = false,                                        \
                },                                                             \
        };                                                                     \
    }                                                                          \
    char __taskio_dummy_fn

#define await_get(future, out)                                                 \
    {                                                                          \
        __TASKIO_FUTURE_OBJ->env.__polling = true;                             \
        struct taskio_task* __TASKIO_TASK_OBJ = taskio_task_new(future);       \
                                                                               \
        while (1) {                                                            \
            enum taskio_future_poll __TASKIO_TASK_POL = TASKIO_FUTURE_PENDING; \
            taskio_task_poll(__TASKIO_TASK_OBJ, __TASKIO_FUTURE_CTX,           \
                             &__TASKIO_TASK_POL, out);                         \
                                                                               \
            if (__TASKIO_TASK_POL == TASKIO_FUTURE_PENDING) {                  \
                swapcontext(&__TASKIO_FUTURE_OBJ->poll_ucp,                    \
                            __TASKIO_FUTURE_OBJ->exec_ucp);                    \
            } else if (__TASKIO_TASK_POL == TASKIO_FUTURE_READY) {             \
                break;                                                         \
            }                                                                  \
                                                                               \
            if (__TASKIO_FUTURE_OBJ->env.__dropped) {                          \
                taskio_task_drop(__TASKIO_TASK_OBJ);                           \
                swapcontext(&__TASKIO_FUTURE_OBJ->poll_ucp,                    \
                            &__TASKIO_FUTURE_OBJ->drop_ucp);                   \
            }                                                                  \
        }                                                                      \
                                                                               \
        taskio_task_drop(__TASKIO_TASK_OBJ);                                   \
        __TASKIO_FUTURE_OBJ->env.__polling = false;                            \
    }

#define await_fn_get(fn, out, ...)                                             \
    {                                                                          \
        struct fn##_future __future_await_obj = fn(__VA_ARGS__);               \
        await_get(__future_await_obj, out);                                    \
    }

#define await(future) await_get(future, NULL)

#define await_fn(fn, ...) await_fn_get(fn, NULL, __VA_ARGS__)

#define async_return(value)                                                    \
    *__TASKIO_FUTURE_POL = TASKIO_FUTURE_READY;                                \
    if (__TASKIO_FUTURE_VAL) {                                                 \
        *__TASKIO_FUTURE_VAL = value;                                          \
    }                                                                          \
    return;

#endif // TASKIO_ASYNC_GUARD_HEADER