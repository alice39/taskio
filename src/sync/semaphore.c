#include <stdatomic.h>
#include <stdlib.h>

#include <pthread.h>

#include "taskio/sync/semaphore.h"

#include "taskio/common.h"
#include "taskio/task/future.h"

struct semaphore_node {
    struct taskio_waker waker;

    struct semaphore_node* back;
    struct semaphore_node* next;
};

struct taskio_semaphore {
    atomic_size_t counter;

    pthread_mutex_t wake_guard;
    struct semaphore_node* wake_queue_head;
    struct semaphore_node* wake_queue_tail;
};

static void
taskio_semaphore_wait_poll(struct taskio_semaphore_wait_future* future,
                           struct taskio_future_context* ctx,
                           enum taskio_future_poll* poll, void* _out);

static void taskio_semaphore_timedwait_poll(
    struct taskio_semaphore_timedwait_future* future,
    struct taskio_future_context* ctx, enum taskio_future_poll* poll,
    bool* out);

static void
taskio_semaphore_wait_drop(struct taskio_semaphore_wait_future* future);

static void taskio_semaphore_timedwait_drop(
    struct taskio_semaphore_timedwait_future* future);

struct taskio_semaphore* taskio_semaphore_new(size_t permits) {
    struct taskio_semaphore* semaphore =
        malloc(sizeof(struct taskio_semaphore));

    if (semaphore) {
        semaphore->counter = permits;

        pthread_mutex_init(&semaphore->wake_guard, NULL);
        semaphore->wake_queue_head = NULL;
        semaphore->wake_queue_tail = NULL;
    }

    return semaphore;
}

void taskio_semaphore_drop(struct taskio_semaphore* semaphore) {
    pthread_mutex_destroy(&semaphore->wake_guard);
    free(semaphore);
}

size_t ttaskio_semaphore_getvalue(struct taskio_semaphore* semaphore) {
    return semaphore->counter;
}

struct taskio_semaphore_wait_future
taskio_semaphore_wait(struct taskio_semaphore* semaphore) {
    return (struct taskio_semaphore_wait_future){
        .poll = taskio_semaphore_wait_poll,
        .drop = taskio_semaphore_wait_drop,
        .env =
            {
                .semaphore = semaphore,
                .current = NULL,
            },
    };
}

struct taskio_semaphore_timedwait_future
taskio_semaphore_timedwait(struct taskio_semaphore* semaphore, uint64_t seconds,
                           uint64_t nanoseconds) {
    struct taskio_semaphore_wait_future* wait_future =
        malloc(sizeof(struct taskio_semaphore_wait_future));
    struct taskio_sleep_future* sleep_future =
        malloc(sizeof(struct taskio_sleep_future));

    *wait_future = taskio_semaphore_wait(semaphore);
    *sleep_future = taskio_sleep(seconds, nanoseconds);

    struct taskio_select_future select_future =
        taskio_select(true, NULL, wait_future, sleep_future);

    return (struct taskio_semaphore_timedwait_future){
        .poll = taskio_semaphore_timedwait_poll,
        .drop = taskio_semaphore_timedwait_drop,
        .env =
            {
                .wait_future = wait_future,
                .sleep_future = sleep_future,
                .task = taskio_task_new(select_future),
            },
    };
}

void taskio_semaphore_signal(struct taskio_semaphore* semaphore) {
    semaphore->counter++;

    pthread_mutex_lock(&semaphore->wake_guard);

    struct semaphore_node* node = semaphore->wake_queue_head;
    if (node) {
        semaphore->wake_queue_head = node->next;
        if (semaphore->wake_queue_head == NULL) {
            semaphore->wake_queue_tail = NULL;
        }
    }

    pthread_mutex_unlock(&semaphore->wake_guard);

    if (node) {
        node->waker.wake(node->waker.data);
    }
}

static void
taskio_semaphore_wait_poll(struct taskio_semaphore_wait_future* future,
                           struct taskio_future_context* ctx,
                           enum taskio_future_poll* poll, void* _out) {
    // supress unsed paramter
    (void)_out;

    struct taskio_semaphore* semaphore = future->env.semaphore;
    size_t current_value = 0;

    do {
        current_value = semaphore->counter;
        if (current_value == 0) {
            break;
        }
    } while (!atomic_compare_exchange_weak(&semaphore->counter, &current_value,
                                           current_value - 1));

    if (current_value == 0) {
        pthread_mutex_lock(&semaphore->wake_guard);

        struct semaphore_node* node = malloc(sizeof(struct semaphore_node));
        node->waker = ctx->waker;
        node->back = semaphore->wake_queue_tail;
        node->next = NULL;

        future->env.current = node;

        if (semaphore->wake_queue_head == NULL) {
            semaphore->wake_queue_head = node;
            semaphore->wake_queue_tail = node;
        } else {
            semaphore->wake_queue_tail->next = node;
            semaphore->wake_queue_tail = node;
        }

        pthread_mutex_unlock(&semaphore->wake_guard);

        *poll = TASKIO_FUTURE_PENDING;
        swapcontext(&future->poll_ucp, future->exec_ucp);
    }

    *poll = TASKIO_FUTURE_READY;
}

static void taskio_semaphore_timedwait_poll(
    struct taskio_semaphore_timedwait_future* future,
    struct taskio_future_context* ctx, enum taskio_future_poll* poll,
    bool* out) {
    size_t result = -1;

    while (1) {
        taskio_task_poll(future->env.task, ctx, poll, &result);
        if (*poll == TASKIO_FUTURE_READY) {
            break;
        } else {
            swapcontext(&future->poll_ucp, future->exec_ucp);
        }
    }

    *out = result == 0;
}

static void
taskio_semaphore_wait_drop(struct taskio_semaphore_wait_future* future) {
    struct semaphore_node* current = future->env.current;
    if (current == NULL) {
        return;
    }

    future->env.current = NULL;

    pthread_mutex_lock(&future->env.semaphore->wake_guard);

    if (current->back) {
        current->back->next = current->next;
    } else {
        future->env.semaphore->wake_queue_head = current->next;
    }

    if (current->next) {
        current->next->back = current->back;
    } else {
        future->env.semaphore->wake_queue_tail = current->back;
    }

    free(current);

    pthread_mutex_unlock(&future->env.semaphore->wake_guard);
}

static void taskio_semaphore_timedwait_drop(
    struct taskio_semaphore_timedwait_future* future) {
    if (future->env.task == NULL) {
        return;
    }

    taskio_task_drop(future->env.task);

    free(future->env.wait_future);
    free(future->env.sleep_future);

    future->env.task = NULL;
    future->env.wait_future = NULL;
    future->env.sleep_future = NULL;
}
