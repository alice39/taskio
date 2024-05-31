#include <stddef.h>
#include <stdlib.h>

#include <semaphore.h>
#include <unistd.h>

#include "taskio/runtime/simple_runtime.h"

#include "runtime_ext.h"

struct taskio_simple_runtime {
    struct task_node* ready_queue_head;
    struct task_node* ready_queue_tail;

    size_t ready_queue_len;
    size_t sleep_queue_len;

    bool is_wating_wake;
    sem_t on_wake;
};

static inline void _runtime_resume(struct taskio_simple_runtime* runtime);

// function for handling taskio_runtime#spawn
static inline struct taskio_join_handle
_runtime_spawn(void* raw_runtime, struct taskio_task* task);

// function for handling taskio_runtime#spawn_blocking
static inline struct taskio_join_handle
_runtime_spawn_blocking(void* raw_runtime, struct taskio_task* task);

static inline struct taskio_waker _waker_clone(void* data);
static inline void _waker_wake(void* raw_data);
static inline void _waker_wake_by_ref(void* raw_data);
static inline void _waker_drop(void* raw_data);

struct taskio_simple_runtime* taskio_simple_runtime_new() {
    struct taskio_simple_runtime* runtime =
        malloc(sizeof(struct taskio_simple_runtime));

    sem_t on_wake;
    int on_wake_result = sem_init(&on_wake, 0, 0);

    if (runtime && on_wake_result == 0) {
        runtime->ready_queue_head = NULL;
        runtime->ready_queue_tail = NULL;
        runtime->ready_queue_len = 0;
        runtime->sleep_queue_len = 0;
        runtime->is_wating_wake = false;
        runtime->on_wake = on_wake;
    } else {
        if (on_wake_result == 0) {
            sem_destroy(&on_wake);
        }

        free(runtime);
        runtime = NULL;
    }

    return runtime;
}

void taskio_simple_runtime_drop(struct taskio_simple_runtime* runtime) {
    struct task_node* node = runtime->ready_queue_head;

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

    // resume event loop if ever got waiting on wake signal.
    _runtime_resume(runtime);

    if (runtime->ready_queue_head == NULL) {
        runtime->ready_queue_head = node;
        runtime->ready_queue_tail = node;
    } else {
        runtime->ready_queue_tail->next = node;
        runtime->ready_queue_tail = node;
    }

    runtime->ready_queue_len++;

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

    while (runtime->ready_queue_head || runtime->sleep_queue_len > 0) {
        // if there's no ready tasks to poll, wait for it. This way the thread
        // will sleep and won't use any CPU usage until given signal is posted.

        if (runtime->ready_queue_head == NULL) {
            sem_wait(&runtime->on_wake);
            continue;
        }

        // once all tasks have been polled, event loop should sleep to avoid CPU
        // consuming to be high

        if (polled_len >= runtime->ready_queue_len) {
            usleep(1);
            polled_len = 0;
        }

        polled_len++;

        // tasks should be popped out of queue when polling them and if given
        // task is still pending, then it will be pushed to the end of queue,
        // but if task is already ready then it will free its memory and won't
        // be added to queue ever again

        struct task_node* node = runtime->ready_queue_head;
        runtime->ready_queue_head = node->next;
        if (runtime->ready_queue_head == NULL) {
            runtime->ready_queue_tail = NULL;
        }
        node->next = NULL;

        runtime->ready_queue_len--;
        runtime->sleep_queue_len++;

        struct task_waker* waker = malloc(sizeof(struct task_waker));
        waker->ref_counter = 1;
        waker->has_woken = false;
        waker->runtime = runtime;
        waker->node = node;

        struct taskio_future_context context = {
            .waker =
                {
                    .clone = _waker_clone,
                    .wake = _waker_wake,
                    .wake_by_ref = _waker_wake_by_ref,
                    .drop = _waker_drop,
                    .data = waker,
                },
        };

        enum taskio_future_poll poll_result = TASKIO_FUTURE_PENDING;
        taskio_task_poll(node->task, &context, &poll_result, NULL);

        switch (poll_result) {
            case TASKIO_FUTURE_READY: {
                context.waker.drop(context.waker.data);
                taskio_task_drop(node->task);
                free(node);
                break;
            }
            case TASKIO_FUTURE_PENDING: {
                // Nothing to do, waiting for task to wake up.
                break;
            }
        }
    }

    taskio_context_pop_runtime();
}

static inline void _runtime_resume(struct taskio_simple_runtime* runtime) {
    if (!runtime->is_wating_wake) {
        return;
    }

    runtime->is_wating_wake = false;
    sem_post(&runtime->on_wake);
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

static inline struct taskio_waker _waker_clone(void* data) {
    struct task_waker* waker = data;
    waker->ref_counter++;

    return (struct taskio_waker){
        .clone = _waker_clone,
        .wake = _waker_wake,
        .wake_by_ref = _waker_wake_by_ref,
        .drop = _waker_drop,
        .data = waker,
    };
}

static inline void _waker_wake(void* data) {
    struct task_waker* waker = data;
    if (waker->has_woken) {
        return;
    }

    waker->has_woken = true;

    _waker_wake_by_ref(data);
    _waker_drop(data);
}

static inline void _waker_wake_by_ref(void* data) {
    struct task_waker* waker = data;
    struct taskio_simple_runtime* runtime = waker->runtime;

    runtime->ready_queue_len++;
    runtime->sleep_queue_len--;

    if (runtime->ready_queue_head == NULL) {
        runtime->ready_queue_head = waker->node;
        runtime->ready_queue_tail = waker->node;
    } else {
        runtime->ready_queue_tail->next = waker->node;
        runtime->ready_queue_tail = waker->node;
    }

    _runtime_resume(runtime);
}

static inline void _waker_drop(void* data) {
    struct task_waker* waker = data;
    waker->ref_counter--;

    if (waker->ref_counter > 0) {
        return;
    }

    struct taskio_simple_runtime* runtime = waker->runtime;
    // only resume event loop if it is suspended by this task.
    _runtime_resume(runtime);

    if (!waker->has_woken) {
        runtime->sleep_queue_len--;
    }

    free(waker);
}
