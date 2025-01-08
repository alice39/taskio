#include <stdio.h>
#include <threads.h>

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

    struct wait_task_future async_tasks[2];
    thrd_t sync_tasks[2];
};

int blocking_task(void* args) {
    struct taskio_mutex* mtx = args;

    printf("blocking_task: lock\n");
    taskio_mutex_blocking_lock(mtx);

    printf("blocking_task: sleeping for 2 seconds.\n");
    thrd_sleep(
        &(struct timespec){
            .tv_sec = 2,
            .tv_nsec = 0,
        },
        NULL);

    printf("blocking_task: unlock\n");
    taskio_mutex_unlock(mtx);
    return 0;
}

taskio_main() {
    struct taskio_mutex* mtx = &async_env(mtx);

    struct wait_task_future* async_tasks = async_env(async_tasks);
    thrd_t* sync_tasks = async_env(sync_tasks);

    async_scope() {
        taskio_mutex_init(mtx);
        thrd_create(sync_tasks + 0, blocking_task, mtx);
        thrd_create(sync_tasks + 1, blocking_task, mtx);

        async_tasks[0] = wait_task(mtx, 3000);
        async_tasks[1] = wait_task(mtx, 1000);
        await_fn(taskio_join, &async_tasks[0], &async_tasks[1]);
    }

    async_scope() {
        thrd_join(sync_tasks[0], NULL);
        thrd_join(sync_tasks[1], NULL);
        taskio_mutex_drop(mtx);
        async_return();
    }
}
