#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "taskio/task/task.h"

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#define STACK_SIZE (16 * 1024)

struct taskio_task {
    struct taskio_future* future;

    bool owned;
    bool polling;
    bool should_drop;

    ucontext_t local_ucp;
    ucontext_t* future_exec_ucp;

#ifdef USE_VALGRIND
    int stack_id;
#endif
    uint8_t stack[STACK_SIZE];
};

struct taskio_task*(taskio_task_new)(struct taskio_future* future,
                                     size_t bytes) {
    struct taskio_task* task = malloc(sizeof(struct taskio_task));
    struct taskio_future* pinned_future = malloc(bytes);

    if (task && pinned_future) {
        task->future = pinned_future;

        task->owned = true;
        task->polling = false;
        task->should_drop = true;

#ifdef USE_VALGRIND
        task->stack_id =
            VALGRIND_STACK_REGISTER(task->stack, task->stack + STACK_SIZE);
#endif

        memcpy(task->future, future, bytes);

        task->future_exec_ucp = NULL;
        pinned_future->exec_ucp = &task->local_ucp;
    } else {
        free(task);
        free(pinned_future);

        task = NULL;
    }

    return task;
}

struct taskio_task* taskio_task_ref(struct taskio_future* future) {
    struct taskio_task* task = malloc(sizeof(struct taskio_task));

    if (task) {
        task->future = future;

        task->owned = false;
        task->polling = false;
        task->should_drop = true;

        task->future_exec_ucp = future->exec_ucp;
        task->future->exec_ucp = &task->local_ucp;
    } else {
        free(task);

        task = NULL;
    }

    return task;
}

void taskio_task_drop(struct taskio_task* task) {
    if (task->should_drop) {
        getcontext(&task->future->drop_ucp);

        task->future->drop_ucp.uc_stack.ss_sp = task->stack;
        task->future->drop_ucp.uc_stack.ss_size = STACK_SIZE;
        task->future->drop_ucp.uc_stack.ss_flags = 0;
        task->future->drop_ucp.uc_link = task->future->exec_ucp;

        makecontext(&task->future->drop_ucp, (void (*)(void))task->future->drop,
                    1, task->future);
        swapcontext(task->future->exec_ucp, &task->future->drop_ucp);
    }

#ifdef USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(task->stack_id);
#endif

    if (task->owned) {
        free(task->future);
    }
    free(task);
}

void taskio_task_poll(struct taskio_task* task,
                      struct taskio_future_context* ctx,
                      enum taskio_future_poll* poll, void* value) {
    if (!task->polling) {
        getcontext(&task->future->poll_ucp);

        task->future->poll_ucp.uc_stack.ss_sp = task->stack;
        task->future->poll_ucp.uc_stack.ss_size = STACK_SIZE;
        task->future->poll_ucp.uc_stack.ss_flags = 0;
        task->future->poll_ucp.uc_link = task->future->exec_ucp;

        makecontext(&task->future->poll_ucp, (void (*)(void))task->future->poll,
                    4, task->future, ctx, poll, value);
    }

    swapcontext(task->future->exec_ucp, &task->future->poll_ucp);

    task->polling = *poll == TASKIO_FUTURE_PENDING;
    task->should_drop = *poll == TASKIO_FUTURE_PENDING;
}
