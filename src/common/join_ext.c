#include <stdarg.h>

#include "../common_ext.h"

static void _join_wake(struct taskio_waker* waker);

future_fn_impl(size_t, taskio_join_ext)(struct taskio_allocator* allocator_ref, void* out, size_t len,
                                        struct taskio_join_task* head, taskio_join_on_ready on_ready,
                                        taskio_join_on_finish on_finish, taskio_join_on_cleanup on_cleanup) {
    struct taskio_allocator allocator = *allocator_ref;
    struct taskio_join_task* poll_head = len >= 1 ? &head[0] : NULL;
    struct taskio_join_task* poll_tail = len >= 1 ? &head[len - 1] : NULL;

    return_future_fn(size_t, taskio_join_ext, allocator, out, len, head, poll_head, poll_tail, on_ready, on_finish,
                     on_cleanup);
}

async_fn(size_t, taskio_join_ext) {

    async_cleanup() {
        taskio_join_on_cleanup on_cleanup = async_env(on_cleanup);
        if (on_cleanup) {
            on_cleanup(&__TASKIO_FUTURE_OBJ->env);
        }

        struct taskio_join_task* head = async_env(head);
        size_t len = async_env(len);
        size_t pending_len = len - async_env(completed_len);

        for (size_t i = 0; i < len && pending_len > 0; i++) {
            struct taskio_future* future = head[i].future;
            if (future == NULL) {
                continue;
            }

            __TASKIO_FUTURE_CLEANUP(future);
            pending_len -= 1;
        }

        async_env(allocator).free(async_env(allocator).data, head);
    }

    async_scope_while(true) {
        async_env(waker) = __TASKIO_FUTURE_CTX->waker;

        struct taskio_join_task* join_task = async_env(poll_head);
        async_env(poll_head) = NULL;
        async_env(poll_tail) = NULL;

        taskio_join_on_ready on_ready = async_env(on_ready);
        taskio_join_on_finish on_finish = async_env(on_finish);

        while (join_task) {
            struct taskio_join_task* next_join_task = join_task->next;

            join_task->env = &__TASKIO_FUTURE_OBJ->env;
            join_task->future->counter += 1;

            struct taskio_future_context ctx = *__TASKIO_FUTURE_CTX;
            ctx.waker.wake = _join_wake;
            ctx.waker.data = join_task;

            enum taskio_future_poll poll = taskio_future_undefined;
            join_task->future->poll(join_task->future, &ctx, &poll, async_env(out));

            switch (poll) {
                case taskio_future_undefined: {
                    TASKIO_TRACE_UNDEFINED(join_task->future);
                    break;
                }
                case taskio_future_pending: {
                    break;
                }
                case taskio_future_ready: {
                    if (on_ready) {
                        on_ready(&__TASKIO_FUTURE_OBJ->env, join_task->future, join_task->index);
                    }

                    __TASKIO_FUTURE_CLEANUP(join_task->future);

                    async_env(completed_len) += 1;
                    join_task->future = NULL;
                    break;
                }
            }

            join_task = next_join_task;
        }

        if (on_finish) {
            size_t index_result = 0;
            bool should_return = on_finish(&__TASKIO_FUTURE_OBJ->env, &index_result);

            if (should_return) {
                async_return(index_result);
            }
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
