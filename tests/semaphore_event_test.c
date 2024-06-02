#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>
#include <taskio/common.h>
#include <taskio/sync/semaphore.h>

struct taskio_semaphore* sem;

async_fn(void, listener, {
    printf("listener: wait\n");
    await_fn(taskio_semaphore_wait, sem);

    uint64_t rand_second = (rand() % 5) + 1;
    printf("listener: sleeping %lu seconds.\n", rand_second);
    await_fn(taskio_sleep, rand_second, 0);
});

async_fn(void, shoot, {
    printf("shoot: signal\n");
    taskio_semaphore_signal(sem);
});

taskio_main({
    sem = taskio_semaphore_new(0);

    struct listener_future f1 = listener();
    struct shoot_future f2 = shoot();
    await_fn(taskio_join, &f1, &f2);

    taskio_semaphore_drop(sem);
});
