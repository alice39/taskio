#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct foo_env {
    int _ignored;
};

static_future_fn(void, foo)() { return_future_fn(void, foo); }

static async_fn(void, foo) {
    async_fn_begin(void, foo);
    async_scope() {}
}

struct taskio_main_env {
    int argc;
    char** args;

    future_env(foo);
};

taskio_main() {
    taskio_main_begin();

    async_scope() {
#ifndef TASKIO_TRACING
        fprintf(stderr, "main: TASKIO_TRACING feature has not been enabled\n");
#endif

        printf("main: waiting for foo\n");
        await_fn(foo);
    }

    async_scope() {
        printf("main: this should not be reached\n");
        async_return();
    }
}
