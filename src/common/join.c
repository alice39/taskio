#include <stdarg.h>

#include <taskio/alloc.h>

#include "../common_ext.h"

future_fn_impl_redirect(void, taskio_join)(struct taskio_allocator* allocator, void* out, size_t len, ...) {
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

    struct taskio_join_ext_future future = taskio_join_ext(allocator, out, len, head, NULL, NULL, NULL);
    return (struct taskio_join_future){
        .inner = future.inner,
        .env =
            {
                .inner = future.env,
            },
    };
}

struct taskio_join_future taskio_join_from_list(struct taskio_allocator* allocator, void* out, size_t len,
                                                struct taskio_future** futures) {
    struct taskio_join_task* head = allocator->alloc(allocator->data, sizeof(struct taskio_join_task) * len);

    for (size_t i = 0; i < len; i++) {
        struct taskio_join_task* join_task = &head[i];

        join_task->index = i;
        join_task->future = futures[i];
        join_task->next = i + 1 < len ? &head[i + 1] : NULL;
    }

    struct taskio_join_ext_future future = taskio_join_ext(allocator, out, len, head, NULL, NULL, NULL);
    return (struct taskio_join_future){
        .inner = future.inner,
        .env =
            {
                .inner = future.env,
            },
    };
}
