#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>

async_fn(void, task, {
    uint64_t rand_second = (rand() % 5) + 1;
    printf("task: sleeping %lu seconds.\n", rand_second);
    await_fn(taskio_sleep, rand_second, 0);
});

taskio_main({
    struct task_future task_one = task();
    struct task_future task_two = task();

    size_t result = -1;
    await_fn_get(taskio_select, &result, false, NULL, &task_one, &task_two);

    printf("\nTask %lu: finished first!\n", result + 1);
});
