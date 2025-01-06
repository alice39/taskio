#include <taskio/runtime.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../runtime_ext.h"
#include "platform_runtime.h"

static __thread uint64_t next_handle_id = 1;

static int worker_run(void* arg);

static void wheel_loop_handler(struct taskio_wheel_timer* wheel_timer);
static void wheel_expiry_handler(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer);

static void task_wake(struct taskio_waker* waker);

void taskio_runtime_init(struct taskio_runtime* runtime, size_t worker_size) {
    switch (worker_size) {
        case TASKIO_SINGLE_THREADED: {
            break;
        }
        case TASKIO_MULTI_THREADED: {
            // TODO: get CPU threads info
            worker_size = 24;
            break;
        }
    }

    runtime->worker_size = worker_size;
    for (size_t i = 0; i < worker_size; i++) {
        struct taskio_worker* worker = runtime->workers + i;

        thrd_create(&worker->id, worker_run, NULL);
        worker->runtime = runtime;

        worker->handle_id = -1;
        worker->handle_out = NULL;
    }

    runtime->platform = malloc(sizeof(struct taskio_platform_runtime));
    runtime->platform->event_fd = eventfd(0, 0);
    runtime->platform->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    runtime->platform->epoll_fd = epoll_create1(0);

    timerfd_settime(runtime->platform->timer_fd, 0,
                    &(struct itimerspec){
                        .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                        .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                    },
                    NULL);

    epoll_ctl(runtime->platform->epoll_fd, EPOLL_CTL_ADD, runtime->platform->event_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->platform->event_fd});

    epoll_ctl(runtime->platform->epoll_fd, EPOLL_CTL_ADD, runtime->platform->timer_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->platform->timer_fd});

    taskio_wheel_timer_init(&runtime->platform->wheels[0], 0, 1, 10, wheel_loop_handler, wheel_expiry_handler, runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[1], 1, 10, 10, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[2], 2, 100, 10, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[3], 3, 1000, 60, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[4], 4, 60000, 60, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[5], 5, 3600000, 24, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[6], 6, 86400000, 365, wheel_loop_handler, wheel_expiry_handler,
                            runtime);
    taskio_wheel_timer_init(&runtime->platform->wheels[7], 7, 31536000000, 4, NULL, wheel_expiry_handler, runtime);

    runtime->poll_head = NULL;
    runtime->poll_tail = NULL;
}

void taskio_runtime_drop(struct taskio_runtime* runtime) {
    close(runtime->platform->event_fd);
    close(runtime->platform->timer_fd);
    close(runtime->platform->epoll_fd);

    for (size_t i = 0; i < 8; i++) {
        taskio_wheel_timer_drop(&runtime->platform->wheels[i]);
    }

    free(runtime->platform);
}

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size) {
    uint64_t handle_id = next_handle_id++;

    struct taskio_task* task = malloc(sizeof(struct taskio_task));
    task->id = handle_id;
    if (future_size == 0) {
        task->pinned = false;
        task->future = future;
    } else {
        task->pinned = true;
        task->future = malloc(future_size);
        memmove(task->future, future, future_size);
    }
    task->next = NULL;

    if (runtime->poll_tail == NULL) {
        runtime->poll_head = task;
        runtime->poll_tail = task;
    } else {
        runtime->poll_tail->next = task;
        runtime->poll_tail = task;
    }

    eventfd_write(runtime->platform->event_fd, 1);

    return (struct taskio_handle){
        .id = handle_id,
        .join = NULL,
    };
}

struct taskio_handle taskio_runtime_spawn_blocking(struct taskio_runtime* runtime, struct taskio_future* future,
                                                   size_t future_size) {
    // FIXME: implement spawn blocking
    (void)runtime;
    (void)future;
    (void)future_size;

    return (struct taskio_handle){};
}

void taskio_runtime_block_on(struct taskio_runtime* runtime, struct taskio_future* future) {
    struct taskio_handle handle = taskio_runtime_spawn(runtime, future, 0);
    struct taskio_worker worker = {.runtime = runtime, .handle_id = handle.id};

    worker_run(&worker);
}

static int worker_run(void* arg) {
    struct taskio_worker* worker = arg;
    bool running = true;

    while (running) {
        struct epoll_event events[1024];
        int nfds = epoll_wait(worker->runtime->platform->epoll_fd, events, 1024, -1);

        for (int i = 0; i < nfds; i++) {
            struct epoll_event* event = &events[i];

            if (event->data.fd == worker->runtime->platform->event_fd) {
                eventfd_t event_out;
                eventfd_read(worker->runtime->platform->event_fd, &event_out);

                struct taskio_task* task = worker->runtime->poll_head;

                worker->runtime->poll_head = worker->runtime->poll_head->next;
                if (worker->runtime->poll_head == NULL) {
                    worker->runtime->poll_tail = NULL;
                }

                task->awaken = false;
                task->next = NULL;

                task->future->counter += 1;

                struct taskio_future_context context = {
                    .waker =
                        {
                            .wake = task_wake,
                            .worker = worker,
                            .task = task,
                        },
                };

                enum taskio_future_poll poll = TASKIO_FUTURE_PENDING;
                task->future->poll(task->future, &context, &poll, worker->handle_out);

                switch (poll) {
                    case TASKIO_FUTURE_READY: {
                        if (worker->handle_id == task->id) {
                            running = false;
                        }
                        if (task->pinned) {
                            free(task->future);
                        }
                        free(task);
                        break;
                    }
                    case TASKIO_FUTURE_PENDING: {
                        // Nothing to do, waiting for future to wake up.
                        break;
                    }
                }
            } else if (event->data.fd == worker->runtime->platform->timer_fd) {
                uint64_t expirations = 0;
                if (read(worker->runtime->platform->timer_fd, &expirations, sizeof(uint64_t)) != sizeof(uint64_t)) {
                    perror("timerfd read");
                }

                struct taskio_wheel_timer* wheel_timer = &worker->runtime->platform->wheels[0];
                while (expirations > 0) {
                    taskio_wheel_timer_tick(wheel_timer);
                    expirations -= 1;
                }
            }
        }
    }

    return 0;
}

static void wheel_loop_handler(struct taskio_wheel_timer* wheel_timer) {
    struct taskio_runtime* runtime = wheel_timer->data;
    taskio_wheel_timer_tick(&runtime->platform->wheels[wheel_timer->id + 1]);
}

static void wheel_expiry_handler(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer) {
    struct taskio_runtime* runtime = wheel_timer->data;
    taskio_runtime_add_timer_from(runtime, timer);
}

static void task_wake(struct taskio_waker* waker) {
    struct taskio_worker* worker = waker->worker;
    struct taskio_runtime* runtime = worker->runtime;

    struct taskio_task* task = waker->task;
    if (task->awaken) {
        return;
    }

    task->awaken = true;

    if (runtime->poll_tail == NULL) {
        runtime->poll_head = task;
        runtime->poll_tail = task;
    } else {
        runtime->poll_tail->next = task;
        runtime->poll_tail = task;
    }

    eventfd_write(runtime->platform->event_fd, 1);
}
