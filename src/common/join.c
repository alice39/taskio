#include <taskio/common.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

future_fn_impl(void, taskio_join)(size_t len, ...) {
    struct taskio_join_future future = return_future_fn_obj(void, taskio_join, len);

    va_list args;
    va_start(args);

    if (len > 16) {
        future.env.is_stack = false;
        future.env.heap_futures = malloc(sizeof(struct taskio_future*) * len);
        for (size_t i = 0; i < len; i++) {
            future.env.heap_futures[i] = va_arg(args, struct taskio_future*);
        }
    } else {
        future.env.is_stack = true;
        for (size_t i = 0; i < len; i++) {
            future.env.stack_futures[i] = va_arg(args, struct taskio_future*);
        }
    }

    va_end(args);

    return future;
}

async_fn(void, taskio_join) {
    async_fn_begin(void, taskio_join);

    async_scope_while(true) {
        bool completed = true;
        bool is_stack = async_env(is_stack);

        for (size_t current = 0; current < async_env(len); current++) {
            struct taskio_future* future = is_stack ? async_env(stack_futures)[current]
                                                    : async_env(heap_futures)[current];
            if (future == NULL) {
                continue;
            }

            future->counter += 1;

            enum taskio_future_poll poll = TASKIO_FUTURE_PENDING;
            future->poll(future, __TASKIO_FUTURE_CTX, &poll, NULL);

            switch (poll) {
                case TASKIO_FUTURE_READY: {
                    if (is_stack) {
                        async_env(stack_futures)[current] = NULL;
                    } else {
                        async_env(heap_futures)[current] = NULL;
                    }
                    break;
                }
                case TASKIO_FUTURE_PENDING: {
                    completed = false;
                    break;
                }
            }
        }

        if (completed) {
            async_break;
        }

        suspended_yield();
    }

    async_scope() { async_return(); }
}
