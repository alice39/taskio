#include <stdarg.h>

#include <taskio/common.h>

struct taskio_join_task {
    struct taskio_future* future;

    struct taskio_join_env* env;
    struct taskio_join_task* next;
};

static void _join_wake(struct taskio_waker* waker);

static void taskio_join_poll(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);

future_fn_impl(void, taskio_join)(struct taskio_allocator* allocator, size_t len, ...) {
    struct taskio_join_task* head = allocator->alloc(allocator->data, sizeof(struct taskio_join_task) * len);
    struct taskio_join_future future = return_future_fn_obj(void, taskio_join, len, head);

    future.env.allocator = *allocator;

    va_list args;
    va_start(args);

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* join_task = &head[i];
        join_task->future = va_arg(args, struct taskio_future*);
        join_task->next = NULL;

        if (future.env.poll_tail) {
            future.env.poll_tail->next = join_task;
        } else {
            future.env.poll_head = join_task;
        }
        future.env.poll_tail = join_task;
    }

    va_end(args);

    return future;
}

struct taskio_join_future taskio_join_from_list(struct taskio_allocator* allocator, size_t len,
                                                struct taskio_future** futures) {
    struct taskio_join_task* head = allocator->alloc(allocator->data, sizeof(struct taskio_join_task) * len);
    struct taskio_join_future future = return_future_fn_obj(void, taskio_join, len, head);

    future.env.allocator = *allocator;

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* join_task = &head[i];
        join_task->future = futures[i];
        join_task->next = NULL;

        if (future.env.poll_tail) {
            future.env.poll_tail->next = join_task;
        } else {
            future.env.poll_head = join_task;
        }
        future.env.poll_tail = join_task;
    }

    return future;
}

async_fn(void, taskio_join) {
    async_cleanup() {
        struct taskio_join_task* head = async_env(head);
        size_t len = async_env(len);
        size_t completed_len = async_env(completed_len);

        for (size_t i = 0; i < len && completed_len > 0; i++) {
            struct taskio_future* future = head[i].future;
            if (future == NULL) {
                continue;
            }

            __TASKIO_FUTURE_CLEANUP(future);
            completed_len -= 1;
        }

        async_env(allocator).free(async_env(allocator).data, head);
    }

    async_scope_while(true) {
        async_env(waker) = __TASKIO_FUTURE_CTX->waker;

        struct taskio_join_task* join_task = async_env(poll_head);
        async_env(poll_head) = NULL;
        async_env(poll_tail) = NULL;

        while (join_task) {
            struct taskio_join_task* next_join_task = join_task->next;

            join_task->env = &__TASKIO_FUTURE_OBJ->env;
            join_task->future->counter += 1;

            struct taskio_future_context ctx = *__TASKIO_FUTURE_CTX;
            ctx.waker.wake = _join_wake;
            ctx.waker.data = join_task;

            enum taskio_future_poll poll = taskio_future_undefined;
            join_task->future->poll(join_task->future, &ctx, &poll, NULL);

            switch (poll) {
                case taskio_future_undefined: {
                    TASKIO_TRACE_UNDEFINED(join_task->future);
                    break;
                }
                case taskio_future_pending: {
                    break;
                }
                case taskio_future_ready: {
                    __TASKIO_FUTURE_CLEANUP(join_task->future);

                    async_env(completed_len) += 1;
                    join_task->future = NULL;
                    break;
                }
            }

            join_task = next_join_task;
        }

        if (async_env(len) == async_env(completed_len)) {
            async_return();
        } else {
            suspended_yield();
        }
    }
}

static void _join_wake(struct taskio_waker* waker) {
    struct taskio_join_task* join_task = waker->data;

    join_task->next = NULL;

    if (join_task->env->poll_tail) {
        join_task->env->poll_tail->next = join_task;
    } else {
        join_task->env->poll_head = join_task;
    }
    join_task->env->poll_tail = join_task;

    join_task->env->waker.wake(&join_task->env->waker);
}
