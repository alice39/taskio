#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/semaphore.h>

struct taskio_semaphore* sem;

async_fn(void, wait_task, {
    printf("task: wait\n");
    await_fn(taskio_semaphore_wait, sem);

    uint64_t rand_second = (rand() % 5) + 1;
    printf("task: sleeping %lu seconds.\n", rand_second);
    await_fn(taskio_sleep, rand_second, 0);

    printf("task: signal\n");
    taskio_semaphore_signal(sem);
});

taskio_main({
    sem = taskio_semaphore_new(1);

    struct wait_task_future wait_one = wait_task();
    struct wait_task_future wait_two = wait_task();
    await_fn(taskio_join, &wait_one, &wait_two);

    taskio_semaphore_drop(sem);
});
