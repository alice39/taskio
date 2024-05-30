#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "runtime_ext.h"
#include "taskio/runtime/simple_runtime.h"

struct task_node {
    struct taskio_task* task;
    struct task_node* next;
};

struct taskio_simple_runtime {
    struct task_node* task_queue_head;
    struct task_node* task_queue_tail;

    size_t task_queue_len;
};

static inline struct taskio_join_handle
_runtime_spawn(void* raw_runtime, struct taskio_task* task);

static inline struct taskio_join_handle
_runtime_spawn_blocking(void* raw_runtime, struct taskio_task* task);

struct taskio_simple_runtime* taskio_simple_runtime_new() {
    struct taskio_simple_runtime* runtime =
        malloc(sizeof(struct taskio_simple_runtime));

    if (runtime) {
        runtime->task_queue_head = NULL;
        runtime->task_queue_tail = NULL;
        runtime->task_queue_len = 0;
    }

    return runtime;
}

void taskio_simple_runtime_drop(struct taskio_simple_runtime* runtime) {
    struct task_node* node = runtime->task_queue_head;

    while (node) {
        taskio_task_drop(node->task);

        struct task_node* next = node->next;
        free(node);
        node = next;
    }

    free(runtime);
}

struct taskio_join_handle
taskio_simple_runtime_spawn(struct taskio_simple_runtime* runtime,
                            struct taskio_task* task) {
    struct task_node* node = malloc(sizeof(struct task_node));
    node->task = task;
    node->next = NULL;

    if (runtime->task_queue_head == NULL) {
        runtime->task_queue_head = node;
        runtime->task_queue_tail = node;
    } else {
        runtime->task_queue_tail->next = node;
        runtime->task_queue_tail = node;
    }

    runtime->task_queue_len++;

    return (struct taskio_join_handle){0};
}

struct taskio_join_handle
taskio_simple_runtime_spawn_blocking(struct taskio_simple_runtime* runtime,
                                     struct taskio_task* task) {
    // FIXME: implement spawn blocking
    (void)runtime;
    (void)task;
    return (struct taskio_join_handle){0};
}

void taskio_simple_runtime_run(struct taskio_simple_runtime* runtime) {
    struct taskio_runtime runtime_ctx = {
        .inner = runtime,
        .spawn = _runtime_spawn,
        .spawn_blocking = _runtime_spawn_blocking,
    };
    taskio_context_push_runtime(&runtime_ctx);

    size_t polled_len = 0;

    while (runtime->task_queue_head) {
        // sleep event loop
        if (polled_len >= runtime->task_queue_len) {
            usleep(1);
            polled_len = 0;
        }

        polled_len++;
        // pop node out of queue
        struct task_node* node = runtime->task_queue_head;
        runtime->task_queue_head = node->next;
        if (runtime->task_queue_head == NULL) {
            runtime->task_queue_tail = NULL;
        }
        node->next = NULL;

        struct taskio_future_context context = {0};
        enum taskio_future_poll poll_result = TASKIO_FUTURE_PENDING;
        taskio_task_poll(node->task, &context, &poll_result, NULL);

        switch (poll_result) {
            case TASKIO_FUTURE_READY: {
                taskio_task_drop(node->task);
                free(node);

                runtime->task_queue_len--;
                break;
            }
            case TASKIO_FUTURE_PENDING: {
                if (runtime->task_queue_head == NULL) {
                    runtime->task_queue_head = node;
                    runtime->task_queue_tail = node;
                } else {
                    runtime->task_queue_tail->next = node;
                    runtime->task_queue_tail = node;
                }
                break;
            }
        }
    }

    taskio_context_pop_runtime();
}

static inline struct taskio_join_handle
_runtime_spawn(void* raw_runtime, struct taskio_task* task) {
    struct taskio_simple_runtime* runtime = raw_runtime;
    return taskio_simple_runtime_spawn(runtime, task);
}

static inline struct taskio_join_handle
_runtime_spawn_blocking(void* raw_runtime, struct taskio_task* task) {
    struct taskio_simple_runtime* runtime = raw_runtime;
    return taskio_simple_runtime_spawn_blocking(runtime, task);
}
