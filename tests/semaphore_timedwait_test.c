#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/semaphore.h>

struct taskio_semaphore* sem;

async_fn(void, timed_task, {
    printf("task: wait\n");

    bool success = false;
    await_fn_get(taskio_semaphore_timedwait, &success, sem, 2, 0);

    if (success) {
        uint64_t rand_second = (rand() % 5) + 1;
        printf("task: sleeping %lu seconds.\n", rand_second);
        await_fn(taskio_sleep, rand_second, 0);

        printf("task: signal\n");
        taskio_semaphore_signal(sem);
    } else {
        printf("task: timed out\n");
    }
});

taskio_main({
    sem = taskio_semaphore_new(1);

    struct timed_task_future timed_one = timed_task();
    struct timed_task_future timed_two = timed_task();
    await_fn(taskio_join, &timed_one, &timed_two);

    taskio_semaphore_drop(sem);
});
