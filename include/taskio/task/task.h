#ifndef TASKIO_TASK_GUARD_HEADER
#define TASKIO_TASK_GUARD_HEADER

#include <stdint.h>

#include "future.h"

#define taskio_task_new(f)                                                     \
    (taskio_task_new)((struct taskio_future*)&f, sizeof(f))

struct taskio_task;

struct taskio_task*(taskio_task_new)(struct taskio_future* future,
                                     size_t bytes);
struct taskio_task* taskio_task_ref(struct taskio_future* future);
void taskio_task_drop(struct taskio_task* task);

void taskio_task_poll(struct taskio_task* task,
                      struct taskio_future_context* ctx,
                      enum taskio_future_poll* poll, void* value);

#endif // TASKIO_TASK_GUARD_HEADER
