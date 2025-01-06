#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_sleep);
};

taskio_main() {
    taskio_main_begin();

    async_scope() {
        printf("main: sleeping for 1 second\n");
        await_fn(taskio_sleep, 1000);
    }

    async_scope() {
        printf("main: slept for 1 second\n");
        async_return();
    }
}
