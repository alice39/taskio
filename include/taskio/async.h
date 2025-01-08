#ifndef TASKIO_ASYNC_GUARD_HEADER
#define TASKIO_ASYNC_GUARD_HEADER

#include <taskio/future.h>

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

#define __TASKIO_FUTURE_CLR_VAL SIZE_MAX

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

#define async_env(name) __TASKIO_FUTURE_OBJ->env.name

#define future_fn(T, name)                                                                                             \
    struct name##_future {                                                                                             \
        struct taskio_future inner;                                                                                    \
        struct name##_env env;                                                                                         \
    };                                                                                                                 \
                                                                                                                       \
    struct name##_future(name)

#define heap_future_fn(T, name) future_fn(T, name)

#define future_fn_impl(T, name, ...)                                                                                   \
    static void name##_poll(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);    \
                                                                                                                       \
    struct name##_future(name)

#define static_future_fn(T, name, ...)                                                                                 \
    struct name##_future {                                                                                             \
        struct taskio_future inner;                                                                                    \
        struct name##_env env;                                                                                         \
    };                                                                                                                 \
                                                                                                                       \
    static void name##_poll(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);    \
                                                                                                                       \
    struct name##_future(name)

#define return_future_fn_obj(T, name, ...)                                                                             \
    (struct name##_future) {                                                                                           \
        .inner = return_future_fn_inner_obj(T, name), .env = (struct name##_env){FUTURE_ENV_INIT(__VA_ARGS__)},        \
    }

#ifdef TASKIO_TRACING
#define return_future_fn_inner_obj(T, future_name)                                                                     \
    (struct taskio_future) {                                                                                           \
        .trace =                                                                                                       \
            {                                                                                                          \
                .name = #future_name,                                                                                  \
                .file = __FILE__,                                                                                      \
                .line = __LINE__,                                                                                      \
            },                                                                                                         \
        .poll = future_name##_poll, .await_future = NULL, .await_out = NULL, .counter = 0,                             \
    }
#else
#define return_future_fn_inner_obj(T, future_name)                                                                     \
    (struct taskio_future) { .poll = future_name##_poll, .await_future = NULL, .await_out = NULL, .counter = 0, }
#endif // TASKIO_TRACING

#define return_future_fn(T, name, ...) return return_future_fn_obj(T, name, __VA_ARGS__)

#define drop_future_fn(future)                                                                                         \
    future.inner.counter = __TASKIO_FUTURE_CLR_VAL;                                                                    \
    future.inner.poll(&future.inner, NULL, NULL, NULL);

#define async_fn(T, name)                                                                                              \
    static void name##_poll1(                                                                                          \
        struct name##_future* __TASKIO_FUTURE_OBJ, struct taskio_future_context* __TASKIO_FUTURE_CTX,                  \
        enum taskio_future_poll* __TASKIO_FUTURE_POL, T* __TASKIO_FUTURE_VAL, size_t __TASKIO_FUTURE_CON);             \
                                                                                                                       \
    static void name##_poll(struct taskio_future* __TASKIO_FUTURE_OBJ_ANY,                                             \
                            struct taskio_future_context* __TASKIO_FUTURE_CTX,                                         \
                            enum taskio_future_poll* __TASKIO_FUTURE_POL, void* __TASKIO_FUTURE_VAL_ANY) {             \
        async_fn_begin(T, name);                                                                                       \
        name##_poll1(__TASKIO_FUTURE_OBJ, __TASKIO_FUTURE_CTX, __TASKIO_FUTURE_POL, __TASKIO_FUTURE_VAL_ANY, 0);       \
    }                                                                                                                  \
                                                                                                                       \
    static void name##_poll1(struct name##_future* __TASKIO_FUTURE_OBJ,                                                \
                             [[maybe_unused]] struct taskio_future_context* __TASKIO_FUTURE_CTX,                       \
                             enum taskio_future_poll* __TASKIO_FUTURE_POL, [[maybe_unused]] T* __TASKIO_FUTURE_VAL,    \
                             size_t __TASKIO_FUTURE_CON)

#define async_fn_begin(T, name)                                                                                        \
    struct name##_future* __TASKIO_FUTURE_OBJ = (struct name##_future*)__TASKIO_FUTURE_OBJ_ANY;                        \
                                                                                                                       \
    struct taskio_future* __TASKIO_TASK_OBJ = __TASKIO_FUTURE_OBJ->inner.await_future;                                 \
    void* __TASKIO_TASK_VAL = __TASKIO_FUTURE_OBJ->inner.await_out;                                                    \
                                                                                                                       \
    if (__TASKIO_TASK_OBJ) {                                                                                           \
        if (__TASKIO_FUTURE_CLR_VAL == __TASKIO_FUTURE_OBJ->inner.counter) {                                           \
            __TASKIO_TASK_OBJ->counter = __TASKIO_FUTURE_CLR_VAL;                                                      \
            __TASKIO_TASK_OBJ->poll(__TASKIO_TASK_OBJ, NULL, NULL, NULL);                                              \
        } else {                                                                                                       \
            __TASKIO_TASK_OBJ->counter += 1;                                                                           \
                                                                                                                       \
            enum taskio_future_poll __TASKIO_TASK_POL = taskio_future_undefined;                                       \
            __TASKIO_TASK_OBJ->poll(__TASKIO_TASK_OBJ, __TASKIO_FUTURE_CTX, &__TASKIO_TASK_POL, __TASKIO_TASK_VAL);    \
                                                                                                                       \
            switch (__TASKIO_TASK_POL) {                                                                               \
                case taskio_future_undefined: {                                                                        \
                    TASKIO_TRACE_UNDEFINED(&__TASKIO_FUTURE_OBJ->inner);                                               \
                    __TASKIO_FUTURE_OBJ->inner.counter -= 1;                                                           \
                    suspended_yield();                                                                                 \
                }                                                                                                      \
                case taskio_future_pending: {                                                                          \
                    __TASKIO_FUTURE_OBJ->inner.counter -= 1;                                                           \
                    suspended_yield();                                                                                 \
                }                                                                                                      \
                case taskio_future_ready: {                                                                            \
                    __TASKIO_TASK_OBJ->counter = __TASKIO_FUTURE_CLR_VAL;                                              \
                    __TASKIO_TASK_OBJ->poll(__TASKIO_TASK_OBJ, NULL, NULL, NULL);                                      \
                    __TASKIO_FUTURE_OBJ->inner.await_future = NULL;                                                    \
                    break;                                                                                             \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    }

#define async_scope()                                                                                                  \
    __TASKIO_FUTURE_CON += 1;                                                                                          \
    if (__TASKIO_FUTURE_CON == __TASKIO_FUTURE_OBJ->inner.counter)

#define async_cleanup() if (__TASKIO_FUTURE_CLR_VAL == __TASKIO_FUTURE_OBJ->inner.counter)

#define yield()                                                                                                        \
    *__TASKIO_FUTURE_POL = taskio_future_pending;                                                                      \
    __TASKIO_FUTURE_CTX->waker.wake(&__TASKIO_FUTURE_CTX->waker);                                                      \
    return

#define suspended_yield()                                                                                              \
    *__TASKIO_FUTURE_POL = taskio_future_pending;                                                                      \
    return

#define await_get_handle(handle, out)                                                                                  \
    taskio_handle_join(&handle, &__TASKIO_FUTURE_CTX->waker, out);                                                     \
    taskio_handle_drop(&handle);                                                                                       \
    suspended_yield()

#define await_handle(handle) await_get_handle(handle, NULL)

#define await_get(future, out)                                                                                         \
    __TASKIO_FUTURE_OBJ->inner.await_future = &future.inner;                                                           \
    __TASKIO_FUTURE_OBJ->inner.await_out = out;                                                                        \
                                                                                                                       \
    future.inner.counter = 1;                                                                                          \
    enum taskio_future_poll __TASKIO_TASK_POL = taskio_future_undefined;                                               \
    future.inner.poll(&future.inner, __TASKIO_FUTURE_CTX, &__TASKIO_TASK_POL, out);                                    \
                                                                                                                       \
    switch (__TASKIO_TASK_POL) {                                                                                       \
        case taskio_future_undefined: {                                                                                \
            TASKIO_TRACE_UNDEFINED(&future.inner);                                                                     \
            suspended_yield();                                                                                         \
        }                                                                                                              \
        case taskio_future_pending: {                                                                                  \
            suspended_yield();                                                                                         \
        }                                                                                                              \
        case taskio_future_ready: {                                                                                    \
            __TASKIO_FUTURE_OBJ->inner.await_future = NULL;                                                            \
            __TASKIO_FUTURE_OBJ->inner.await_out = NULL;                                                               \
            yield();                                                                                                   \
        }                                                                                                              \
    }

#define await_fn_get(fn, out, ...)                                                                                     \
    async_env(__TASKIO_FUTURE_ENV).fn = fn(__VA_ARGS__);                                                               \
    await_get(__TASKIO_FUTURE_OBJ->env.__TASKIO_FUTURE_ENV.fn, out);

#define await(future) await_get(future, NULL)

#define await_fn(fn, ...) await_fn_get(fn, NULL, __VA_ARGS__)

#define async_return(...)                                                                                              \
    *__TASKIO_FUTURE_POL = taskio_future_ready;                                                                        \
    __VA_OPT__(if (__TASKIO_FUTURE_VAL) { *__TASKIO_FUTURE_VAL = __VA_ARGS__; })                                       \
    return

#define async_scope_while(cond)                                                                                        \
    __TASKIO_FUTURE_CON += 1;                                                                                          \
    if (__TASKIO_FUTURE_CON == __TASKIO_FUTURE_OBJ->inner.counter)                                                     \
        if (((cond) && ((__TASKIO_FUTURE_OBJ->inner.counter -= 1) | 1)) ||                                             \
            ((__TASKIO_FUTURE_OBJ->inner.counter += 1) & 0))

#define async_break                                                                                                    \
    __TASKIO_FUTURE_OBJ->inner.counter += 1;                                                                           \
    yield()

#endif // TASKIO_ASYNC_GUARD_HEADER
