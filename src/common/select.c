#include <stdarg.h>
#include <stdlib.h>

#include "taskio/common.h"

static inline void taskio_select_poll(struct taskio_select_future* future,
                                      struct taskio_future_context* ctx,
                                      enum taskio_future_poll* poll,
                                      size_t* out) {

    struct taskio_select_env* env = &future->env;
    struct taskio_join_handle* handles = env->handles;

    struct select_node {
        size_t index;
        struct select_node* next;
    };

    struct select_node* finished_list = NULL;
    size_t finished_list_len = 0;

    while (1) {
        for (size_t i = 0; i < env->len; i++) {
            if (!handles[i].is_finished(&handles[i])) {
                continue;
            }

            struct select_node* node = malloc(sizeof(struct select_node));
            node->index = i;
            node->next = finished_list;

            finished_list = node;
            finished_list_len++;

            if (env->biased) {
                break;
            }
        }

        if (finished_list) {
            break;
        }

        *poll = TASKIO_FUTURE_PENDING;
        ctx->waker.wake(ctx->waker.data);
        swapcontext(future->poll_ucp, future->exec_ucp);
    }

    // TODO: Use a RNG instead of PRNG
    size_t random_choice = rand() % finished_list_len;

    while (random_choice > 0) {
        struct select_node* next = finished_list->next;
        free(finished_list);
        finished_list = next;
        random_choice--;
    }

    size_t index_result = finished_list->index;

    struct select_node* node = finished_list;
    while (node) {
        struct select_node* next = node->next;
        free(node);
        node = next;
    }

    for (size_t i = 0; i < env->len; i++) {
        struct taskio_join_handle* handle = &handles[i];
        handle->abort(handle);

        while (1) {
            if (handle->is_destroyed(handle)) {
                break;
            }

            *poll = TASKIO_FUTURE_PENDING;
            ctx->waker.wake(ctx->waker.data);
            swapcontext(future->poll_ucp, future->exec_ucp);
        }

        taskio_join_handle_drop(handle);
    }

    // FIXME: return the value of finished future
    *poll = TASKIO_FUTURE_READY;
    *out = index_result;
    free(handles);
}

void taskio_select_drop(struct taskio_select_future* future) {
    for (size_t i = 0; i < future->env.len; i++) {
        struct taskio_join_handle* handle = &future->env.handles[i];
        handle->abort(handle);
        taskio_join_handle_drop(handle);
    }

    free(future->env.handles);
}

struct taskio_select_future(taskio_select)(bool biased, void* out, size_t len,
                                           ...) {
    va_list args;
    va_start(args, len);

    struct taskio_join_handle* handles =
        malloc(sizeof(struct taskio_join_handle) * len);

    for (size_t i = 0; i < len; i++) {
        struct taskio_future* future = va_arg(args, struct taskio_future*);
        handles[i] = taskio_runtime_spawn(taskio_task_ref(future));
    }

    va_end(args);

    return (struct taskio_select_future){
        .poll = taskio_select_poll,
        .drop = taskio_select_drop,
        .env =
            {
                .biased = biased,
                .out = out,
                .handles = handles,
                .len = len,
            },
    };
}
