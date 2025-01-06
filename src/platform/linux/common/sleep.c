#include <taskio/common.h>

#include "../../../runtime_ext.h"

static void taskio_sleep_handler(void*);

future_fn_impl(void, taskio_sleep)(uint64_t ms) { return_future_fn(void, taskio_sleep, ms); }

async_fn(void, taskio_sleep) {
    async_fn_begin(void, taskio_sleep);

    async_scope() {
        struct taskio_future_context* ctx = &async_env(ctx);
        *ctx = *__TASKIO_FUTURE_CTX;

        struct taskio_worker* worker = __TASKIO_FUTURE_CTX->waker.worker;
        taskio_runtime_add_timer(worker->runtime, async_env(ms), taskio_sleep_handler, ctx);
        suspended_yield();
    }

    async_scope() { async_return(); }
}

static void taskio_sleep_handler(void* data) {
    struct taskio_future_context* context = data;
    context->waker.wake(&context->waker);
}
