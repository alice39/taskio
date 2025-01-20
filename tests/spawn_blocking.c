#include <stdio.h>
#include <threads.h>

#include <taskio.h>
#include <taskio/common.h>

struct task_data {
    int id;
    uint64_t delay;
};

void task(void* arg, [[maybe_unused]] void* out) {
    struct task_data* data = arg;

    printf("task %d: sleeping for %lu second\n", data->id, data->delay / 1000);
    thrd_sleep(&(struct timespec){.tv_sec = data->delay / 1000, .tv_nsec = (data->delay % 1000) * 1000000}, NULL);
    printf("task %d: slept for %lu second\n", data->id, data->delay / 1000);
}

struct taskio_main_env {
    int argc;
    char** args;

    struct task_data data[3];

    struct taskio_handle h2;
};

taskio_main() {
    async_scope() {
        async_env(data)[0].id = 0;
        async_env(data)[0].delay = 1000;
        async_env(data)[1].id = 1;
        async_env(data)[1].delay = 3000;
        async_env(data)[2].id = 2;
        async_env(data)[2].delay = 1000;

        taskio_spawn_blocking(task, &async_env(data)[0]);

        async_env(h2) = taskio_spawn_blocking_with_handle(task, &async_env(data)[1]);

        struct taskio_handle h3 = taskio_spawn_blocking_with_handle(task, &async_env(data)[2]);
        await_handle(h3);
    }

    async_scope() {
        struct taskio_handle h2 = async_env(h2);
        // it won't affect if task has started
        taskio_handle_abort(&h2);
        taskio_handle_drop(&h2);

        async_return();
    }
}
