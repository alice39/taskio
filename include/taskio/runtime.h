#ifndef TASKIO_RUNTIME_GUARD_HEADER
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <threads.h>

#include "future.h"

#define TASKIO_SINGLE_THREADED (0)
#define TASKIO_MULTI_THREADED (-1)

#define taskio_spawn_pinned(future)                                                                                    \
    taskio_runtime_spawn(((struct taskio_worker*)__TASKIO_FUTURE_CTX->worker)->runtime, &future.inner, 0)

#define taskio_spawn_with_handle(future)                                                                               \
    taskio_runtime_spawn(((struct taskio_worker*)__TASKIO_FUTURE_CTX->worker)->runtime, &future.inner, sizeof(future))

#define taskio_spawn(future)                                                                                           \
    {                                                                                                                  \
        struct taskio_handle __taskio_handle = taskio_spawn_with_handle(future);                                       \
        taskio_handle_drop(&__taskio_handle);                                                                          \
    }

struct taskio_runtime;

struct taskio_task_wake_node;

struct taskio_task {
    atomic_size_t counter;
    uint64_t id;

    bool pinned;
    bool awaken;
    bool aborted;
    bool finished;

    struct taskio_runtime* runtime;
    struct taskio_task_wake_node* wake_on_ready_top;

    struct taskio_future* future;
    struct taskio_task* next;
};

struct taskio_worker {
    thrd_t id;
    struct taskio_runtime* runtime;

    uint64_t handle_id; // -1 if undefined
    void* handle_out;   // NULL if undefined
};

struct taskio_platform_runtime;

struct taskio_runtime {
    size_t worker_size;
    struct taskio_worker workers[24];

    struct taskio_platform_runtime* platform;

    bool poll_scheduled;
    struct taskio_task* poll_head;
    struct taskio_task* poll_tail;
};

struct taskio_handle {
    uint64_t id;
    void* task;
};

void taskio_runtime_init(struct taskio_runtime* runtime, size_t workers);
void taskio_runtime_drop(struct taskio_runtime* runtime);

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size);
struct taskio_handle taskio_runtime_spawn_blocking(struct taskio_runtime* runtime, struct taskio_future* future,
                                                   size_t future_size);

void taskio_runtime_block_on(struct taskio_runtime* runtime, struct taskio_future* future);

struct taskio_handle taskio_handle_clone(struct taskio_handle* handle);
void taskio_handle_drop(struct taskio_handle* handle);

bool taskio_handle_is_aborted(struct taskio_handle* handle);
bool taskio_handle_is_finished(struct taskio_handle* handle);

void taskio_handle_abort(struct taskio_handle* handle);
void taskio_handle_join(struct taskio_handle* handle, struct taskio_waker* waker, void* out);

#endif // TASKIO_RUNTIME_GUARD_HEADER
