#ifndef TASKIO_RUNTIME_GUARD_HEADER
#define TASKIO_RUNTIME_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <threads.h>

#include "future.h"

#define TASKIO_SINGLE_THREADED (0)
#define TASKIO_MULTI_THREADED (-1)

struct taskio_task {
    uint64_t id;
    bool pinned;
    bool can_jmp[16];
    jmp_buf jmp[16];
    struct taskio_future* future;
    struct taskio_task* next;
};

struct taskio_worker {
    thrd_t id;
    struct taskio_runtime* runtime;

    uint64_t handle_id; // -1 if undefined
    void* handle_out;   // NULL if undefined
};

struct taskio_runtime {
    size_t worker_size;
    struct taskio_worker workers[24];

    int event_fd;
    int epoll_fd;

    struct taskio_task* poll_head;
    struct taskio_task* poll_tail;
};

struct taskio_handle {
    uint64_t id;
    void (*join)();
};

void taskio_runtime_init(struct taskio_runtime* runtime, size_t workers);
void taskio_runtime_drop(struct taskio_runtime* runtime);

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size);
struct taskio_handle taskio_runtime_spawn_blocking(struct taskio_runtime* runtime, struct taskio_future* future,
                                                   size_t future_size);

void taskio_runtime_block_on(struct taskio_runtime* runtime, struct taskio_future* future);

#endif // TASKIO_RUNTIME_GUARD_HEADER
