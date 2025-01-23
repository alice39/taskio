#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

struct taskio_main_env {
    int argc;
    char** args;

    future_env(taskio_select);

    struct taskio_sleep_future f1;
    struct taskio_sleep_future f2;
    size_t select_index;
};

taskio_main() {
    async_scope() {
        async_env(f1) = taskio_sleep(5000);
        async_env(f2) = taskio_sleep(1000);

        printf("main: select two futures\n");
        await_fn_get(&async_env(select_index), taskio_select(NULL, &async_env(f1), &async_env(f2)));
    }

    async_scope() {
        printf("main: won %lu\n", async_env(select_index));
        async_return();
    }
}
