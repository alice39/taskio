#include <taskio/common.h>

#include "../../../runtime_ext.h"

static void taskio_sleep_handler(void*);

future_fn_impl(void, taskio_sleep)(uint64_t ms) { return_future_fn(void, taskio_sleep, ms); }

async_fn(void, taskio_sleep) {
    async_fn_begin(void, taskio_sleep);

    async_cleanup() {
        struct taskio_timer* timer = async_env(timer);
        if (timer) {
            taskio_timer_abort(timer);
        }
    }

    async_scope() {
        struct taskio_waker* waker = &async_env(waker);
        *waker = __TASKIO_FUTURE_CTX->waker;

        struct taskio_worker* worker = __TASKIO_FUTURE_CTX->worker;
        async_env(timer) = taskio_runtime_add_timer(worker->runtime, async_env(ms), taskio_sleep_handler, waker);
        suspended_yield();
    }

    async_scope() { async_return(); }
}

static void taskio_sleep_handler(void* data) {
    struct taskio_waker* waker = data;
    waker->wake(waker);
}
