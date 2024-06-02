#ifndef TASKIO_RUNTIME_EXT_GUARD_HEADER
#define TASKIO_RUNTIME_EXT_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>

#include "taskio/runtime/runtime.h"

struct taskio_runtime {
    void* inner;

    struct taskio_join_handle (*spawn)(void* inner, struct taskio_task* task);
    struct taskio_join_handle (*spawn_blocking)(void* inner,
                                                struct taskio_task* task);
};

struct task_node {
    size_t counter;

    bool aborted;
    bool finished;

    struct taskio_task* task;
    struct task_node* next;
};

struct task_waker {
    size_t ref_counter;
    bool has_woken;

    void* runtime;
    struct task_node* node;
};

struct task_node* taskio_task_node_clone(struct task_node* node);
void taskio_task_node_drop(struct task_node* node);

void taskio_context_push_runtime(struct taskio_runtime* runtime);
void taskio_context_pop_runtime();

#endif // TASKIO_RUNTIME_EXT_GUARD_HEADER
