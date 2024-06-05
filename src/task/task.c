#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "taskio/context.h"
#include "taskio/task/task.h"

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#define STACK_SIZE (2 * 1024)

struct taskio_task {
    struct taskio_future* future;

    bool polling;
    bool should_drop;

    ucontext_t exec_ucp;
    ucontext_t poll_ucp;
    ucontext_t drop_ucp;

#ifdef USE_VALGRIND
    int stack_id;
#endif
    uint8_t stack[STACK_SIZE];
    uint8_t pinned_future[1];
};

struct taskio_task*(taskio_task_new)(struct taskio_future* future,
                                     size_t bytes) {
    struct taskio_task* task = malloc(sizeof(struct taskio_task) + bytes);

    if (task) {
        task->future = (struct taskio_future*)task->pinned_future;

        task->polling = false;
        task->should_drop = true;

#ifdef USE_VALGRIND
        task->stack_id =
            VALGRIND_STACK_REGISTER(task->stack, task->stack + STACK_SIZE);
#endif

        memcpy(task->future, future, bytes);

        task->future->exec_ucp = &task->exec_ucp;
        task->future->poll_ucp = &task->poll_ucp;
        task->future->drop_ucp = &task->drop_ucp;
    }

    return task;
}

struct taskio_task* taskio_task_ref(struct taskio_future* future) {
    struct taskio_task* task = malloc(sizeof(struct taskio_task));

    if (task) {
        task->future = future;

        task->polling = false;
        task->should_drop = true;

        task->future->exec_ucp = &task->exec_ucp;
        task->future->poll_ucp = &task->poll_ucp;
        task->future->drop_ucp = &task->drop_ucp;
    }

    return task;
}

void taskio_task_drop(struct taskio_task* task) {
    if (task->should_drop) {
        getcontext(&task->drop_ucp);

        task->drop_ucp.uc_stack.ss_sp = task->stack;
        task->drop_ucp.uc_stack.ss_size = STACK_SIZE;
        task->drop_ucp.uc_stack.ss_flags = 0;
        task->drop_ucp.uc_link = task->future->exec_ucp;

        makecontext(&task->drop_ucp, (void (*)(void))task->future->drop, 1,
                    task->future);
        swapcontext(&task->exec_ucp, &task->drop_ucp);
    }

#ifdef USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(task->stack_id);
#endif

    free(task);
}

void taskio_task_poll(struct taskio_task* task,
                      struct taskio_future_context* ctx,
                      enum taskio_future_poll* poll, void* value) {
    if (!task->polling) {
        getcontext(&task->poll_ucp);

        task->poll_ucp.uc_stack.ss_sp = task->stack;
        task->poll_ucp.uc_stack.ss_size = STACK_SIZE;
        task->poll_ucp.uc_stack.ss_flags = 0;
        task->poll_ucp.uc_link = task->future->exec_ucp;

        makecontext(&task->poll_ucp, (void (*)(void))task->future->poll, 4,
                    task->future, ctx, poll, value);
    }

    swapcontext(&task->exec_ucp, &task->poll_ucp);

    task->polling = *poll == TASKIO_FUTURE_PENDING;
    task->should_drop = *poll == TASKIO_FUTURE_PENDING;
}
