#ifdef TASKIO_TRACING
#include <stdio.h>
#endif

#include <taskio/sync/mutex.h>

void taskio_mutex_init(struct taskio_mutex* mutex) { taskio_semaphore_init(&mutex->sem, 1); }

void taskio_mutex_init_with_alloc(struct taskio_mutex* mutex, struct taskio_allocator* allocator) {
    taskio_semaphore_init_with_alloc(&mutex->sem, allocator, 1);
}

void taskio_mutex_drop(struct taskio_mutex* mutex) { taskio_semaphore_drop(&mutex->sem); }

bool taskio_mutex_try_lock(struct taskio_mutex* mutex) {
    if (taskio_semaphore_getvalue(&mutex->sem) == 0) {
        return false;
    }

    taskio_semaphore_blocking_wait(&mutex->sem);
    return true;
}

future_fn_impl(void, taskio_mutex_lock)(struct taskio_mutex* mutex) {
    return_future_fn(void, taskio_mutex_lock, mutex);
}

future_fn_impl(bool, taskio_mutex_timedlock)(struct taskio_mutex* mutex, uint64_t delay) {
    return_future_fn(bool, taskio_mutex_timedlock, mutex, delay);
}

async_fn(void, taskio_mutex_lock) {
    async_scope() { await_fn(taskio_semaphore_wait, &async_env(mutex)->sem); }
    async_scope() { async_return(); }
}

async_fn(bool, taskio_mutex_timedlock) {
    async_scope() {
        await_fn_get(taskio_semaphore_timedwait, &async_env(result), &async_env(mutex)->sem, async_env(delay));
    }

    async_scope() { async_return(async_env(result)); }
}

void taskio_mutex_blocking_lock(struct taskio_mutex* mutex) { taskio_semaphore_blocking_wait(&mutex->sem); }

void taskio_mutex_unlock(struct taskio_mutex* mutex) {
#ifdef TASKIO_TRACING
    if (taskio_semaphore_getvalue(&mutex->sem) > 0) {
        fprintf(stderr, "taskio-tracing: mutex was unlocked twice\n");
    }
#endif // TASKIO_TRACING

    taskio_semaphore_signal(&mutex->sem);
}
