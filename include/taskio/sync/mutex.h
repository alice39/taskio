#ifndef TASKIO_SYNC_MUTEX_GUARD_HEADER
#define TASKIO_SYNC_MUTEX_GUARD_HEADER

#include <taskio/async.h>
#include <taskio/sync/semaphore.h>

struct taskio_mutex {
    struct taskio_semaphore sem;
};

struct taskio_mutex_lock_env {
    struct taskio_mutex* mutex;

    future_env(taskio_semaphore_wait);
};

void taskio_mutex_init(struct taskio_mutex* mutex);
void taskio_mutex_init_with_alloc(struct taskio_mutex* mutex, struct taskio_allocator* allocator);
void taskio_mutex_drop(struct taskio_mutex* mutex);

// true if success, otherwise false
bool taskio_mutex_try_lock(struct taskio_mutex* mutex);
future_fn(void, taskio_mutex_lock)(struct taskio_mutex* mutex);
void taskio_mutex_blocking_lock(struct taskio_mutex* mutex);
void taskio_mutex_unlock(struct taskio_mutex* mutex);

#endif // TASKIO_SYNC_SEMAPHORE_GUARD_HEADER
