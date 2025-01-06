#include <taskio/common.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct taskio_join_task {
    struct taskio_future* future;

    struct taskio_join_env* env;
    struct taskio_join_task* next;
};

static void _join_wake(struct taskio_waker* waker);

future_fn_impl(void, taskio_join)(size_t len, ...) {
    struct taskio_join_future future = return_future_fn_obj(void, taskio_join, len);

    va_list args;
    va_start(args);

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* task = malloc(sizeof(struct taskio_join_task));
        task->future = va_arg(args, struct taskio_future*);
        task->next = NULL;

        if (future.env.poll_tail) {
            future.env.poll_tail->next = task;
        } else {
            future.env.poll_head = task;
        }
        future.env.poll_tail = task;
    }

    va_end(args);

    return future;
}

struct taskio_join_future taskio_join_from_list(size_t len, struct taskio_future** futures) {
    struct taskio_join_future future = return_future_fn_obj(void, taskio_join, len);

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* task = malloc(sizeof(struct taskio_join_task));
        task->future = futures[i];
        task->next = NULL;

        if (future.env.poll_tail) {
            future.env.poll_tail->next = task;
        } else {
            future.env.poll_head = task;
        }
        future.env.poll_tail = task;
    }

    return future;
}

async_fn(void, taskio_join) {
    async_fn_begin(void, taskio_join);

    async_scope_while(true) {
        async_env(waker) = __TASKIO_FUTURE_CTX->waker;

        struct taskio_join_task* task = async_env(poll_head);
        async_env(poll_head) = NULL;
        async_env(poll_tail) = NULL;

        while (task) {
            struct taskio_join_task* next_task = task->next;

            task->env = &__TASKIO_FUTURE_OBJ->env;
            task->future->counter += 1;

            struct taskio_future_context ctx = {
                .waker =
                    {
                        .wake = _join_wake,
                        .worker = async_env(waker).worker,
                        .task = task,
                    },
            };
            enum taskio_future_poll poll = TASKIO_FUTURE_PENDING;
            task->future->poll(task->future, &ctx, &poll, NULL);

            switch (poll) {
                case TASKIO_FUTURE_READY: {
                    async_env(len) -= 1;
                    free(task);
                    break;
                }
                case TASKIO_FUTURE_PENDING: {
                    break;
                }
            }

            task = next_task;
        }

        if (async_env(len) > 0) {
            suspended_yield();
        } else {
            async_return();
        }
    }
}

static void _join_wake(struct taskio_waker* waker) {
    struct taskio_join_task* task = waker->task;

    task->next = NULL;

    if (task->env->poll_tail) {
        task->env->poll_tail->next = task;
    } else {
        task->env->poll_head = task;
    }
    task->env->poll_tail = task;

    task->env->waker.wake(&task->env->waker);
}
