#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/semaphore.h>

struct listener_env {
    struct taskio_semaphore* sem;
    future_env(taskio_semaphore_wait, taskio_sleep);
};

struct shoot_env {
    struct taskio_semaphore* sem;
};

static_future_fn(void, listener)(struct taskio_semaphore* sem) { return_future_fn(void, listener, sem); }

static_future_fn(void, shoot)(struct taskio_semaphore* sem) { return_future_fn(void, shoot, sem); }

async_fn(void, listener) {
    async_scope() {
        printf("listener: wait\n");
        await_fn(taskio_semaphore_wait(async_env(sem)));
    }

    async_scope() {
        printf("listener: sleeping for 4 seconds.\n");
        await_fn(taskio_sleep(4000));
    }

    async_scope() { async_return(); }
}

async_fn(void, shoot) {
    async_scope() {
        printf("shoot: signal\n");
        taskio_semaphore_signal(async_env(sem));
        async_return();
    }
}

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_join);

    struct taskio_semaphore sem;
    struct listener_future f1;
    struct shoot_future f2;
};

taskio_main() {
    struct taskio_semaphore* sem = &async_env(sem);

    async_scope() {
        taskio_semaphore_init(sem, 0);

        async_env(f1) = listener(sem);
        async_env(f2) = shoot(sem);
        await_fn(taskio_join(NULL, &async_env(f1), &async_env(f2)));
    }

    async_scope() {
        taskio_semaphore_drop(sem);
        async_return();
    }
}
