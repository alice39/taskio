#ifndef TASKIO_ASYNC_GUARD_HEADER
#define TASKIO_ASYNC_GUARD_HEADER

#include "future.h"

#define __TASKIO_FUTURE_OBJ_ANY __taskio_future
#define __TASKIO_FUTURE_OBJ __taskio_future_own
#define __TASKIO_FUTURE_CTX __taskio_ctx
#define __TASKIO_FUTURE_POL __taskio_poll
#define __TASKIO_FUTURE_VAL_ANY __taskio_val
#define __TASKIO_FUTURE_VAL __taskio_val_own
#define __TASKIO_FUTURE_CON __taskio_counter
#define __TASKIO_FUTURE_ENV __taskio_future

#define __TASKIO_TASK_OBJ __taskio_task_obj
#define __TASKIO_TASK_CTX __taskio_task_ctx
#define __TASKIO_TASK_POL __taskio_task_poll
#define __TASKIO_TASK_VAL __taskio_task_val

#define future_env(...) __VA_OPT__(union {future_env_1(__VA_ARGS__)} __TASKIO_FUTURE_ENV)

#define future_env_1(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_2(__VA_ARGS__))
#define future_env_2(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_3(__VA_ARGS__))
#define future_env_3(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_4(__VA_ARGS__))
#define future_env_4(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_5(__VA_ARGS__))
#define future_env_5(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_6(__VA_ARGS__))
#define future_env_6(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_7(__VA_ARGS__))
#define future_env_7(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_8(__VA_ARGS__))
#define future_env_8(name, ...)                                                                                        \
    struct name##_future name;                                                                                         \
    __VA_OPT__(future_env_9(__VA_ARGS__))
#define future_env_9(name, ...) struct name##_future name;

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

#define async_env_decl(T, name) T, name
#define async_env(name) __TASKIO_FUTURE_OBJ->env.name

#define future_fn(T, name)                                                                                             \
    struct name##_future {                                                                                             \
        struct taskio_future inner;                                                                                    \
        struct name##_env env;                                                                                         \
    };                                                                                                                 \
                                                                                                                       \
    void name##_poll(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);           \
                                                                                                                       \
    struct name##_future(name)

#define future_fn_impl(T, name)                                                                                        \
    void name##_poll(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);           \
                                                                                                                       \
    struct name##_future(name)

#define return_future_fn_obj(T, name, ...)                                                                             \
    (struct name##_future) {                                                                                           \
        .inner = {.poll = name##_poll, .await_future = NULL, .await_out = NULL, .counter = 0},                         \
        .env = (struct name##_env){FUTURE_ENV_INIT(__VA_ARGS__)},                                                      \
    }

#define return_future_fn(T, name, ...) return return_future_fn_obj(T, name, __VA_ARGS__)

#define async_fn(T, name)                                                                                              \
    void name##_poll(struct taskio_future* __TASKIO_FUTURE_OBJ_ANY, struct taskio_future_context* __TASKIO_FUTURE_CTX, \
                     enum taskio_future_poll* __TASKIO_FUTURE_POL, void* __TASKIO_FUTURE_VAL_ANY)

#define async_fn_begin(T, name)                                                                                        \
    struct name##_future* __TASKIO_FUTURE_OBJ = (struct name##_future*)__TASKIO_FUTURE_OBJ_ANY;                        \
    [[maybe_unused]] T* __TASKIO_FUTURE_VAL = __TASKIO_FUTURE_VAL_ANY;                                                 \
    size_t __TASKIO_FUTURE_CON = 0;                                                                                    \
                                                                                                                       \
    struct taskio_future* __TASKIO_TASK_OBJ = __TASKIO_FUTURE_OBJ->inner.await_future;                                 \
    void* __TASKIO_TASK_VAL = __TASKIO_FUTURE_OBJ->inner.await_out;                                                    \
                                                                                                                       \
    if (__TASKIO_TASK_OBJ) {                                                                                           \
        __TASKIO_TASK_OBJ->counter += 1;                                                                               \
                                                                                                                       \
        enum taskio_future_poll __TASKIO_TASK_POL = TASKIO_FUTURE_PENDING;                                             \
        __TASKIO_TASK_OBJ->poll(__TASKIO_TASK_OBJ, __TASKIO_FUTURE_CTX, &__TASKIO_TASK_POL, __TASKIO_TASK_VAL);        \
                                                                                                                       \
        switch (__TASKIO_TASK_POL) {                                                                                   \
            case TASKIO_FUTURE_READY: {                                                                                \
                __TASKIO_FUTURE_OBJ->inner.await_future = NULL;                                                        \
                break;                                                                                                 \
            }                                                                                                          \
            case TASKIO_FUTURE_PENDING: {                                                                              \
                suspended_yield();                                                                                     \
            }                                                                                                          \
        }                                                                                                              \
    }

#define async_scope()                                                                                                  \
    __TASKIO_FUTURE_CON += 1;                                                                                          \
    if (__TASKIO_FUTURE_CON == __TASKIO_FUTURE_OBJ->inner.counter)

#define yield()                                                                                                        \
    *__TASKIO_FUTURE_POL = TASKIO_FUTURE_PENDING;                                                                      \
    __TASKIO_FUTURE_CTX->waker.wake(&__TASKIO_FUTURE_CTX->waker);                                                      \
    return

#define suspended_yield()                                                                                              \
    *__TASKIO_FUTURE_POL = TASKIO_FUTURE_PENDING;                                                                      \
    return

#define await_get(future, out)                                                                                         \
    __TASKIO_FUTURE_OBJ->inner.await_future = &future.inner;                                                           \
    __TASKIO_FUTURE_OBJ->inner.await_out = out;                                                                        \
                                                                                                                       \
    future.inner.counter = 1;                                                                                          \
    enum taskio_future_poll __TASKIO_TASK_POL = TASKIO_FUTURE_PENDING;                                                 \
    future.inner.poll(&future.inner, __TASKIO_FUTURE_CTX, &__TASKIO_TASK_POL, out);                                    \
                                                                                                                       \
    switch (__TASKIO_TASK_POL) {                                                                                       \
        case TASKIO_FUTURE_READY: {                                                                                    \
            __TASKIO_FUTURE_OBJ->inner.await_future = NULL;                                                            \
            __TASKIO_FUTURE_OBJ->inner.await_out = NULL;                                                               \
            yield();                                                                                                   \
        }                                                                                                              \
        case TASKIO_FUTURE_PENDING: {                                                                                  \
            suspended_yield();                                                                                         \
        }                                                                                                              \
    }

#define await_fn_get(fn, out, ...)                                                                                     \
    async_env(__TASKIO_FUTURE_ENV).fn = fn(__VA_ARGS__);                                                               \
    await_get(__TASKIO_FUTURE_OBJ->env.__TASKIO_FUTURE_ENV.fn, out);

#define await(future) await_get(future, NULL)

#define await_fn(fn, ...) await_fn_get(fn, NULL, __VA_ARGS__)

#define async_return(...)                                                                                              \
    *__TASKIO_FUTURE_POL = TASKIO_FUTURE_READY;                                                                        \
    __VA_OPT__(if (__TASKIO_FUTURE_VAL) { *__TASKIO_FUTURE_VAL = __VA_ARGS__; })                                       \
    return

#endif // TASKIO_ASYNC_GUARD_HEADER
