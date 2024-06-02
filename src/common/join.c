#include <stdarg.h>
#include <stdlib.h>

#include "taskio/common.h"

static inline void taskio_join_poll(struct taskio_join_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out) {
    // supress unused parameter
    (void)_out;

    struct taskio_join_env* env = &future->env;
    struct taskio_join_handle* handles = env->handles;

    while (env->current < env->len) {
        struct taskio_join_handle* handle = &handles[env->current];
        if (handle->is_finished(handle)) {
            taskio_join_handle_drop(handle);
            env->current++;
        } else {
            // FIXME: Wake when it's needed to.
            *poll = TASKIO_FUTURE_PENDING;
            ctx->waker.wake(ctx->waker.data);
            swapcontext(&future->poll_ucp, future->exec_ucp);
        }
    }

    free(handles);
    *poll = TASKIO_FUTURE_READY;
}

static inline void taskio_join_drop(struct taskio_join_future* future) {
    for (size_t i = future->env.current; i < future->env.len; i++) {
        struct taskio_join_handle* handle = &future->env.handles[i];
        handle->abort(handle);
        taskio_join_handle_drop(handle);
    }

    free(future->env.handles);
}

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
                .current = 0,
                .len = len,
            },
    };
}
