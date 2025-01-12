#include <taskio/common.h>

#include "../runtime_ext.h"

static void taskio_sleep_handler(void*);

future_fn_impl(void, taskio_sleep)(uint64_t ms) { return_future_fn(void, taskio_sleep, ms); }

async_fn(void, taskio_sleep) {
    struct taskio_timer_handle handle = {
        .id = async_env(timer_id),
        .expiry_time = async_env(timer_expiry_time),
    };

    async_cleanup() {
        if (taskio_timer_valid(&handle)) {
            taskio_runtime_timer_abort(async_env(runtime), &handle);
        }
    }

    async_scope() {
        async_env(waker) = __TASKIO_FUTURE_CTX->waker;
        async_env(runtime) = __TASKIO_FUTURE_CTX->runtime;

        handle = taskio_runtime_add_timer(async_env(runtime), async_env(ms), taskio_sleep_handler, &async_env(waker));

        async_env(timer_id) = handle.id;
        async_env(timer_expiry_time) = handle.expiry_time;

        suspended_yield();
    }

    async_scope() {
        async_env(timer_id) = 0;
        async_env(timer_expiry_time) = 0;

        async_return();
    }
}

static void taskio_sleep_handler(void* data) {
    struct taskio_waker* waker = data;
    waker->wake(waker);
}
