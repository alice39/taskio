#ifndef TASKIO_RUNTIME_GUARD_HEADER
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <taskio/alloc.h>
#include <taskio/future.h>

#define TASKIO_SINGLE_THREADED (0)
#define TASKIO_MULTI_THREADED (-1)

#define taskio_spawn_pinned(future) taskio_runtime_spawn(__TASKIO_FUTURE_CTX->runtime, &future.inner, 0)

#define taskio_spawn_with_handle(future)                                                                               \
    taskio_runtime_spawn(__TASKIO_FUTURE_CTX->runtime, &future.inner, sizeof(future))

#define taskio_spawn(future)                                                                                           \
    {                                                                                                                  \
        struct taskio_handle __taskio_handle = taskio_spawn_with_handle(future);                                       \
        taskio_handle_drop(&__taskio_handle);                                                                          \
    }

typedef char taskio_stack_runtime[6 * 1024];

struct taskio_runtime;

struct taskio_handle {
    uint64_t id;
    void* task;
};

size_t taskio_runtime_size();

void taskio_runtime_init(struct taskio_runtime* runtime, size_t worker_size, struct taskio_allocator* allocator);
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
