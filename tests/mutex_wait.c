#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/mutex.h>

struct wait_task_env {
    struct taskio_mutex* mtx;
    uint64_t delay;

    future_env(taskio_mutex_lock, taskio_sleep);
};

static_future_fn(void, wait_task)(struct taskio_mutex* mtx, uint64_t delay) {
    return_future_fn(void, wait_task, mtx, delay);
}

async_fn(void, wait_task) {
    struct taskio_mutex* mtx = async_env(mtx);
    uint64_t delay = async_env(delay);

    async_scope() {
        printf("task: lock\n");
        await_fn(taskio_mutex_lock, mtx);
    }

    async_scope() {
        printf("task: sleeping for %lu seconds.\n", delay / 1000);
        await_fn(taskio_sleep, delay);
    }

    async_scope() {
        printf("task: unlock\n");
        taskio_mutex_unlock(mtx);
        async_return();
    }
}

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_join);

    struct taskio_mutex mtx;
    struct wait_task_future wait_one;
    struct wait_task_future wait_two;
};

taskio_main() {
    struct taskio_mutex* mtx = &async_env(mtx);

    async_scope() {
        taskio_mutex_init(mtx);

        async_env(wait_one) = wait_task(mtx, 4000);
        async_env(wait_two) = wait_task(mtx, 2000);
        await_fn(taskio_join, &async_env(wait_one), &async_env(wait_two));
    }

    async_scope() {
        taskio_mutex_drop(mtx);
        async_return();
    }
}
