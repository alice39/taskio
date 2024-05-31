#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "taskio/common.h"

static inline void taskio_join_poll(struct taskio_join_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out);

static inline void async_sleep_poll(struct taskio_sleep_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out);

static inline void async_empty_drop(void* _future) {
    // supress unused parameter
    (void)_future;
}

static inline void taskio_join_drop(struct taskio_join_future* future);

struct taskio_join_future(taskio_join)(size_t len, ...) {
    va_list args;
    va_start(args, len);

    struct taskio_join_handle* handles =
        malloc(sizeof(struct taskio_join_handle) * len);

    for (size_t i = 0; i < len; i++) {
        struct taskio_future* future = va_arg(args, struct taskio_future*);
        handles[i] = taskio_runtime_spawn(taskio_task_ref(future));
    }

    va_end(args);

    return (struct taskio_join_future){
        .poll = taskio_join_poll,
        .drop = taskio_join_drop,
        .env =
            {
                .handles = handles,
                .len = len,
            },
    };
}

struct taskio_sleep_future taskio_sleep(uint64_t seconds,
                                        uint64_t nanoseconds) {
    struct timespec current;
    timespec_get(&current, TIME_UTC);

    return (struct taskio_sleep_future){
        .poll = async_sleep_poll,
        .drop = (void (*)(struct taskio_sleep_future*))async_empty_drop,
        .env = {
            .deadline =
                {
                    .tv_sec = current.tv_sec + seconds,
                    .tv_nsec = current.tv_nsec + nanoseconds,
                },
        }};
}

static inline void taskio_join_poll(struct taskio_join_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out) {
    // supress unused parameter
    (void)_out;

    struct taskio_join_env* env = &future->env;
    struct taskio_join_handle* handles = env->handles;

    size_t i = 0;
    while (i < env->len) {
        if (handles[i].is_finished(handles[i].task)) {
            i++;
        } else {
            // FIXME: Wake when it's needed to.
            *poll = TASKIO_FUTURE_PENDING;
            ctx->waker.wake(ctx->waker.data);
        }
    }

    free(handles);
    *poll = TASKIO_FUTURE_READY;
}

static inline void async_sleep_poll(struct taskio_sleep_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out) {
    // supress unused parameter
    (void)_out;

    while (1) {
        struct timespec current;
        timespec_get(&current, TIME_UTC);

        struct timespec* deadline = &future->env.deadline;

        if (current.tv_sec >= deadline->tv_sec &&
            current.tv_nsec >= deadline->tv_nsec) {
            *poll = TASKIO_FUTURE_READY;
        } else {
            *poll = TASKIO_FUTURE_PENDING;
            ctx->waker.wake(ctx->waker.data);
        }

        swapcontext(&future->poll_ucp, future->exec_ucp);
    }
}

static inline void taskio_join_drop(struct taskio_join_future* future) {
    for (size_t i = 0; i < future->env.len; i++) {
        struct taskio_join_handle* handle = &future->env.handles[i];
        handle->abort(handle->task);
    }

    free(future->env.handles);
}
