#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

taskio_main(
    {
        printf("main: sleeping for 1 second\n");
        await_fn(taskio_sleep, 1000);
        printf("main: slept for 1 second\n");
    },
    future_env(taskio_sleep));
