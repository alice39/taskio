#include <stdarg.h>
#include <stdlib.h>

#include <taskio/alloc.h>

#include "../common_ext.h"

static void _on_ready(struct taskio_join_ext_env* env, struct taskio_future* future, size_t index);
static bool _on_finish(struct taskio_join_ext_env* env, size_t* out_index);
static void _on_cleanup(struct taskio_join_ext_env* env);

struct taskio_select_node {
    size_t index;
    struct taskio_select_node* next;
};

future_fn_impl_redirect(size_t, taskio_select)(struct taskio_allocator* allocator, bool biased, void* out, size_t len,
                                               ...) {
    if (allocator == NULL) {
        allocator = taskio_default_allocator();
    }

    struct taskio_join_task* head = allocator->alloc(allocator->data, sizeof(struct taskio_join_task) * len);

    va_list args;
    va_start(args);

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* join_task = &head[i];

        join_task->index = i;
        join_task->future = va_arg(args, struct taskio_future*);
        join_task->next = i + 1 < len ? &head[i + 1] : NULL;
    }

    va_end(args);

    struct taskio_join_ext_future future =
        taskio_join_ext(allocator, out, len, head, _on_ready, _on_finish, _on_cleanup);
    return (struct taskio_select_future){
        .inner = future.inner,
        .env =
            {
                .inner = future.env,
                .biased = biased,
            },
    };
}

static void _on_ready(struct taskio_join_ext_env* inner_env, [[maybe_unused]] struct taskio_future* future,
                      size_t index) {
    struct taskio_select_env* env = (struct taskio_select_env*)inner_env;
    struct taskio_allocator* allocator = inner_env->allocator;

    struct taskio_select_node* select_node = allocator->alloc(allocator->data, sizeof(struct taskio_select_node));
    select_node->index = index;
    select_node->next = NULL;

    env->len += 1;

    if (env->tail == NULL) {
        env->head = select_node;
    } else {
        env->tail->next = select_node;
    }

    env->tail = select_node;
}

static bool _on_finish(struct taskio_join_ext_env* inner_env, size_t* out_index) {
    struct taskio_select_env* env = (struct taskio_select_env*)inner_env;

    struct taskio_select_node* select_node = env->head;
    if (select_node == NULL) {
        return false;
    }

    if (!env->biased) {
        size_t random_skip = ((size_t)rand()) % env->len;
        for (size_t i = 0; i < random_skip; i++) {
            select_node = select_node->next;
        }
    }

    *out_index = select_node->index;

    return true;
}

static void _on_cleanup(struct taskio_join_ext_env* inner_env) {
    struct taskio_select_env* env = (struct taskio_select_env*)inner_env;

    struct taskio_allocator* allocator = env->inner.allocator;

    struct taskio_select_node* select_node = env->head;
    while (select_node) {
        struct taskio_select_node* next_select_node = select_node->next;
        allocator->free(allocator->data, select_node);
        select_node = next_select_node;
    }
}
