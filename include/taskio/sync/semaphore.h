#ifndef TASKIO_SYNC_SEMAPHORE_GUARD_HEADER
#define TASKIO_SYNC_SEMAPHORE_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "../async.h"
#include "../runtime/runtime.h"

struct taskio_semaphore;

struct taskio_semaphore_wait_env {
    struct taskio_semaphore* semaphore;

    void* current;
};

struct taskio_semaphore_timedwait_env {
    struct taskio_semaphore_wait_future* wait_future;
    struct taskio_sleep_future* sleep_future;

    struct taskio_task* task;
};

struct taskio_semaphore* taskio_semaphore_new(size_t permits);
void taskio_semaphore_drop(struct taskio_semaphore* semaphore);

size_t ttaskio_semaphore_getvalue(struct taskio_semaphore* semaphore);

future_fn(void, taskio_semaphore_wait)(struct taskio_semaphore* semaphore);
future_fn(bool, taskio_semaphore_timedwait)(struct taskio_semaphore* semaphore,
                                            uint64_t seconds,
                                            uint64_t nanoseconds);
void taskio_semaphore_signal(struct taskio_semaphore* semaphore);

#endif // TASKIO_SYNC_SEMAPHORE_GUARD_HEADER
