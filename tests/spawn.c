#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct task_env {
    int id;
    uint64_t delay;

    future_env(taskio_sleep);
};

static_future_fn(void, task, future_arg(int, id), future_arg(uint64_t, delay)) {
    return_future_fn(void, task, id, delay);
}

async_fn(void, task) {
    async_fn_begin(void, task);

    async_cleanup() { printf("task %d: cleaned up\n", async_env(id)); }

    async_scope() {
        printf("task %d: sleeping for %lu second\n", async_env(id), async_env(delay) / 1000);
        await_fn(taskio_sleep, async_env(delay));
    }

    async_scope() {
        printf("task %d: slept for %lu second\n", async_env(id), async_env(delay) / 1000);
        async_return();
    }
}

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_sleep);

    struct taskio_handle h2;
};

taskio_main() {
    taskio_main_begin();

    async_scope() {
        struct task_future f1 = task(1, 1000);
        struct task_future f2 = task(2, 3000);
        struct task_future f3 = task(3, 1000);

        taskio_spawn(f1);

        async_env(h2) = taskio_spawn_with_handle(f2);

        struct taskio_handle h3 = taskio_spawn_with_handle(f3);
        await_handle(h3);
    }

    async_scope() {
        struct taskio_handle h2 = async_env(h2);
        taskio_handle_abort(&h2);
        taskio_handle_drop(&h2);

        async_return();
    }
}
