#include <stdio.h>
#include <threads.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/semaphore.h>

struct wait_task_env {
    struct taskio_semaphore* sem;
    uint64_t delay;

    future_env(taskio_semaphore_wait, taskio_sleep);
};

static_future_fn(void, wait_task)(struct taskio_semaphore* sem, uint64_t delay) {
    return_future_fn(void, wait_task, sem, delay);
}

async_fn(void, wait_task) {
    struct taskio_semaphore* sem = async_env(sem);
    uint64_t delay = async_env(delay);

    async_scope() {
        printf("task: wait\n");
        await_fn(taskio_semaphore_wait, sem);
    }

    async_scope() {
        printf("task: sleeping for %lu seconds.\n", delay / 1000);
        await_fn(taskio_sleep, delay);
    }

    async_scope() {
        printf("task: signal\n");
        taskio_semaphore_signal(sem);
        async_return();
    }
}

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_join);

    struct taskio_semaphore sem;

    struct wait_task_future async_tasks[2];
    thrd_t sync_tasks[2];
};

int blocking_task(void* args) {
    struct taskio_semaphore* sem = args;

    printf("blocking_task: wait\n");
    taskio_semaphore_blocking_wait(sem);

    printf("blocking_task: sleeping for 2 seconds.\n");
    thrd_sleep(
        &(struct timespec){
            .tv_sec = 2,
            .tv_nsec = 0,
        },
        NULL);

    printf("blocking_task: signal\n");
    taskio_semaphore_signal(sem);
    return 0;
}

taskio_main() {
    struct taskio_semaphore* sem = &async_env(sem);

    struct wait_task_future* async_tasks = async_env(async_tasks);
    thrd_t* sync_tasks = async_env(sync_tasks);

    async_scope() {
        taskio_semaphore_init(sem, 1);
        thrd_create(sync_tasks + 0, blocking_task, sem);
        thrd_create(sync_tasks + 1, blocking_task, sem);

        async_tasks[0] = wait_task(sem, 3000);
        async_tasks[1] = wait_task(sem, 1000);
        await_fn(taskio_join, &async_tasks[0], &async_tasks[1]);
    }

    async_scope() {
        thrd_join(sync_tasks[0], NULL);
        thrd_join(sync_tasks[1], NULL);
        taskio_semaphore_drop(sem);
        async_return();
    }
}
