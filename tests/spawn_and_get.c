#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct task_env {
    int id;
    uint64_t delay;

    future_env(taskio_sleep);
};

static_future_fn(void, task)(int id, uint64_t delay) { return_future_fn(void, task, id, delay); }

async_fn(int, task) {
    int id = async_env(id);
    uint64_t delay = async_env(delay);

    async_cleanup() { printf("task %d: cleaned up\n", id); }

    async_scope() {
        printf("task %d: sleeping for %lu second\n", id, delay / 1000);
        await_fn(taskio_sleep, async_env(delay));
    }

    async_scope() {
        printf("task %d: slept for %lu second\n", id, delay / 1000);
        async_return(id + delay);
    }
}

struct taskio_main_env {
    int argc;
    char** args;

    int results[2];
};

taskio_main() {
    int* results = async_env(results);

    async_scope() {
        struct task_future f = task(1, 1000);
        struct taskio_handle h = taskio_spawn_with_handle(f, int);
        await_get_handle(h, &results[0]);
    }

    async_scope() {
        struct task_future f = task(2, 2000);
        struct taskio_handle h = taskio_spawn_with_handle(f, int);
        await_get_handle(h, &results[1]);
    }

    async_scope() {
        printf("result 1: %d\n", results[0]);
        printf("result 2: %d\n", results[1]);
        async_return();
    }
}
