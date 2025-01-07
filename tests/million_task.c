#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

#define COUNT 1000000

struct taskio_main_env {
    int argc;
    char** args;

    size_t i;

    struct taskio_sleep_future futures[COUNT];
    struct taskio_handle handles[COUNT];
};

taskio_main() {
    taskio_main_begin();

    async_scope() {
        for (size_t i = 0; i < COUNT; i++) {
            async_env(futures)[i] = taskio_sleep(1000);
            async_env(handles)[i] = taskio_spawn_pinned(async_env(futures)[i]);
        }

        yield();
    }

    size_t i = async_env(i);

    async_scope_while(i < COUNT) {
        struct taskio_handle handle = async_env(handles)[i];
        async_env(i) += 1;

        await_handle(handle);
    }

    async_scope() {
        printf("main: done\n");
        async_return();
    }
}
