#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

taskio_main(future_env(taskio_join), async_env_decl(struct taskio_sleep_future, f1),
            async_env_decl(struct taskio_sleep_future, f2)) {
    taskio_main_begin();

    async_scope() {
        async_env(f1) = taskio_sleep(1000);
        async_env(f2) = taskio_sleep(1000);

        printf("main: joint two futures\n");
        await_fn(taskio_join, &async_env(f1), &async_env(f2));
    }

    async_scope() {
        printf("main: joint finished\n");
        async_return();
    }
}
