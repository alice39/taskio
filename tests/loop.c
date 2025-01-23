#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_sleep);
    size_t i;
};

taskio_main() {
    async_scope_while(async_env(i) < 5) {
        printf("main: sleeping for 500ms (%lu)\n", async_env(i));

        async_env(i)++;
        await_fn(taskio_sleep(500));
    }

    async_scope_while(true) {
        if (async_env(i) >= 10) {
            async_break;
        }

        printf("main: sleeping for 250ms (%lu)\n", async_env(i));

        async_env(i)++;
        await_fn(taskio_sleep(250));
    }

    async_scope() { async_return(); }
}
