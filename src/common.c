#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "taskio/common.h"

static inline void async_gather_poll(struct taskio_join_future* future,
                                     struct taskio_future_context* ctx,
                                     enum taskio_future_poll* poll, void* _out);

static inline void async_sleep_poll(struct taskio_sleep_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out);

static inline void async_empty_drop(void* _future) {
    // supress unused parameter
    (void)_future;
}

static inline void async_gather_drop(struct taskio_join_future* future);

struct taskio_join_future(taskio_join)(size_t len, ...) {
    va_list args;
    va_start(args, len);

    struct taskio_task** tasks = malloc(sizeof(struct task*) * len);
    for (size_t i = 0; i < len; i++) {
        struct taskio_future* future = va_arg(args, struct taskio_future*);
        tasks[i] = taskio_task_ref(future);
    }

    va_end(args);

    return (struct taskio_join_future){
        .poll = async_gather_poll,
        .drop = async_gather_drop,
        .env =
            {
                .tasks = tasks,
                .len = len,
            },
    };
}

struct taskio_sleep_future taskio_sleep(uint64_t seconds,
                                        uint64_t nanoseconds) {
    struct timespec current;
    timespec_get(&current, TIME_UTC);

    return (struct taskio_sleep_future){
        .poll = async_sleep_poll,
        .drop = (void (*)(struct taskio_sleep_future*))async_empty_drop,
        .env = {
            .deadline =
                {
                    .tv_sec = current.tv_sec + seconds,
                    .tv_nsec = current.tv_nsec + nanoseconds,
                },
        }};
}

static inline void async_gather_poll(struct taskio_join_future* future,
                                     struct taskio_future_context* ctx,
                                     enum taskio_future_poll* poll,
                                     void* _out) {
    // supress unused parameter
    (void)ctx;
    (void)_out;

    while (1) {
        struct taskio_join_env* env = &future->env;
        struct taskio_task** tasks = env->tasks;

        size_t ready_count = 0;

        for (size_t i = 0; i < env->len; i++) {
            if (tasks[i] == NULL) {
                ready_count++;
                continue;
            }

            enum taskio_future_poll poll_result = TASKIO_FUTURE_PENDING;
            taskio_task_poll(tasks[i], ctx, &poll_result, NULL);

            switch (poll_result) {
                case TASKIO_FUTURE_READY: {
                    ready_count++;
                    taskio_task_drop(tasks[i]);
                    tasks[i] = NULL;
                    break;
                }
                case TASKIO_FUTURE_PENDING: {
                    // Nothing to do
                    break;
                }
            }
        }

        if (ready_count == env->len) {
            free(future->env.tasks);
            *poll = TASKIO_FUTURE_READY;
        } else {
            *poll = TASKIO_FUTURE_PENDING;
        }

        swapcontext(&future->poll_ucp, future->exec_ucp);
    }
}

static inline void async_sleep_poll(struct taskio_sleep_future* future,
                                    struct taskio_future_context* ctx,
                                    enum taskio_future_poll* poll, void* _out) {
    // supress unused parameter
    (void)ctx;
    (void)_out;

    while (1) {
        struct timespec current;
        timespec_get(&current, TIME_UTC);

        struct timespec* deadline = &future->env.deadline;

        if (current.tv_sec >= deadline->tv_sec &&
            current.tv_nsec >= deadline->tv_nsec) {
            *poll = TASKIO_FUTURE_READY;
        } else {
            *poll = TASKIO_FUTURE_PENDING;
        }

        swapcontext(&future->poll_ucp, future->exec_ucp);
    }
}

static inline void async_gather_drop(struct taskio_join_future* future) {
    for (size_t i = 0; i < future->env.len; i++) {
        struct taskio_task* task = future->env.tasks[i];
        if (task) {
            taskio_task_drop(task);
        }
    }
    free(future->env.tasks);
}
