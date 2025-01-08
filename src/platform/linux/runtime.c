#include <taskio/async.h>
#include <taskio/runtime.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runtime_platform.h"

static __thread uint64_t next_handle_id = 1;

static int worker_run(void* arg);

static void _wheel_loop(struct taskio_wheel_timer* wheel_timer);
static void _wheel_expiry(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer);

static void task_add_event_loop(struct taskio_task* task);
static void task_wake(struct taskio_waker* waker);

static void task_drop(struct taskio_task* task);

size_t taskio_runtime_size() { return sizeof(struct taskio_runtime); }

void taskio_runtime_init(struct taskio_runtime* runtime, size_t worker_size, struct taskio_allocator* allocator) {
    runtime->allocator = *allocator;

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

    runtime->event_fd = eventfd(0, 0);
    runtime->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    runtime->epoll_fd = epoll_create1(0);

    timerfd_settime(runtime->timer_fd, 0,
                    &(struct itimerspec){
                        .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                        .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                    },
                    NULL);

    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, runtime->event_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->event_fd});

    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, runtime->timer_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->timer_fd});

    struct taskio_wheel_timer* wheels = runtime->hierarchy_wheel.wheels;
    struct taskio_timer** timer_buckets = runtime->hierarchy_wheel.buckets;

    taskio_wheel_timer_init(&wheels[0], allocator, 0, 1, 10, &timer_buckets[0], _wheel_loop, _wheel_expiry, runtime);
    taskio_wheel_timer_init(&wheels[1], allocator, 1, 10, 10, &timer_buckets[10], _wheel_loop, _wheel_expiry, runtime);
    taskio_wheel_timer_init(&wheels[2], allocator, 2, 100, 10, &timer_buckets[20], _wheel_loop, _wheel_expiry, runtime);
    taskio_wheel_timer_init(&wheels[3], allocator, 3, 1000, 60, &timer_buckets[30], _wheel_loop, _wheel_expiry,
                            runtime);
    taskio_wheel_timer_init(&wheels[4], allocator, 4, 60000, 60, &timer_buckets[90], _wheel_loop, _wheel_expiry,
                            runtime);
    taskio_wheel_timer_init(&wheels[5], allocator, 5, 3600000, 24, &timer_buckets[150], _wheel_loop, _wheel_expiry,
                            runtime);
    taskio_wheel_timer_init(&wheels[6], allocator, 6, 86400000, 365, &timer_buckets[174], _wheel_loop, _wheel_expiry,
                            runtime);
    taskio_wheel_timer_init(&wheels[7], allocator, 7, 31536000000, 4, &timer_buckets[539], NULL, _wheel_expiry,
                            runtime);

    runtime->poll_scheduled = false;
    runtime->poll_head = NULL;
    runtime->poll_tail = NULL;
}

void taskio_runtime_drop(struct taskio_runtime* runtime) {
    close(runtime->event_fd);
    close(runtime->timer_fd);
    close(runtime->epoll_fd);

    for (size_t i = 0; i < 8; i++) {
        taskio_wheel_timer_drop(&runtime->hierarchy_wheel.wheels[i]);
    }
}

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size) {
    uint64_t handle_id = next_handle_id++;

    struct taskio_task* task =
        runtime->allocator.alloc(runtime->allocator.data, sizeof(struct taskio_task) + future_size);

    // handled by the user and runtime
    task->counter = 2;
    task->id = handle_id;

    task->awaken = false;
    task->status = taskio_task_scheduled;

    task->runtime = runtime;
    task->wake_on_ready_top = NULL;

    task->next = NULL;

    if (future_size == 0) {
        task->future = future;
    } else {
        task->future = (struct taskio_future*)&task->raw_futures[0];
        memcpy(task->future, future, future_size);
    }

    task_add_event_loop(task);

    return (struct taskio_handle){
        .id = handle_id,
        .task = task,
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

    taskio_handle_drop(&handle);
    worker_run(&worker);
}

struct taskio_handle taskio_handle_clone(struct taskio_handle* handle) {
    struct taskio_task* task = handle->task;
    task->counter += 1;

    return (struct taskio_handle){
        .id = handle->id,
        .task = handle->task,
    };
}

void taskio_handle_drop(struct taskio_handle* handle) {
    struct taskio_task* task = handle->task;
    task_drop(task);

    handle->id = 0;
    handle->task = NULL;
}

bool taskio_handle_is_aborted(struct taskio_handle* handle) {
    struct taskio_task* task = handle->task;
    return task->status == taskio_task_aborted;
}

bool taskio_handle_is_finished(struct taskio_handle* handle) {
    struct taskio_task* task = handle->task;
    return task->status == taskio_task_finished;
}

void taskio_handle_abort(struct taskio_handle* handle) {
    struct taskio_task* task = handle->task;
    switch (task->status) {
        case taskio_task_scheduled: {
            break;
        }
        case taskio_task_aborted:
        case taskio_task_finished: {
            return;
        }
    }

    task->status = taskio_task_aborted;
    task_add_event_loop(task);
}

void taskio_handle_join(struct taskio_handle* handle, struct taskio_waker* waker, void* out) {
    struct taskio_task* task = handle->task;
    switch (task->status) {
        case taskio_task_scheduled: {
            break;
        }
        case taskio_task_aborted:
        case taskio_task_finished: {
            waker->wake(waker);
            return;
        }
    }

    struct taskio_task_wake_node* node =
        task->runtime->allocator.alloc(task->runtime->allocator.data, sizeof(struct taskio_task_wake_node));
    node->waker = *waker;
    node->out = out;
    node->next = task->wake_on_ready_top;

    task->wake_on_ready_top = node;
}

static int worker_run(void* arg) {
    struct taskio_worker* worker = arg;
    bool running = true;

    while (running) {
        struct epoll_event events[1024];
        int nfds = epoll_wait(worker->runtime->epoll_fd, events, 1024, -1);

        for (int i = 0; i < nfds; i++) {
            struct epoll_event* event = &events[i];

            if (event->data.fd == worker->runtime->event_fd) {
                eventfd_t event_out;
                eventfd_read(worker->runtime->event_fd, &event_out);

                while (true) {
                    struct taskio_task* task = worker->runtime->poll_head;
                    if (task == NULL) {
                        break;
                    }

                    worker->runtime->poll_head = worker->runtime->poll_head->next;
                    if (worker->runtime->poll_head == NULL) {
                        worker->runtime->poll_tail = NULL;
                    }

                    task->awaken = false;
                    task->next = NULL;

                    if (task->status == taskio_task_aborted) {
                        // cleanup process
                        task->future->counter = __TASKIO_FUTURE_CLR_VAL;
                        task->future->poll(task->future, NULL, NULL, NULL);

                        if (worker->handle_id == task->id) {
                            running = false;
                        }

                        task_drop(task);
                        continue;
                    }

                    task->future->counter += 1;

                    struct taskio_future_context context = {
                        .waker =
                            {
                                .wake = task_wake,
                                .data = task,
                            },
                        .runtime = worker->runtime,
                    };

                    enum taskio_future_poll poll = taskio_future_undefined;
                    task->future->poll(task->future, &context, &poll, worker->handle_out);

                    switch (poll) {
                        case taskio_future_undefined: {
                            TASKIO_TRACE_UNDEFINED(task->future);
                            break;
                        }
                        case taskio_future_pending: {
                            // Nothing to do, waiting for future to wake up.
                            break;
                        }
                        case taskio_future_ready: {
                            // cleanup process
                            task->future->counter = __TASKIO_FUTURE_CLR_VAL;
                            task->future->poll(task->future, NULL, NULL, NULL);

                            task->status = taskio_task_finished;

                            struct taskio_task_wake_node* wake_node = task->wake_on_ready_top;
                            while (wake_node) {
                                struct taskio_task_wake_node* next_wake_node = wake_node->next;

                                wake_node->waker.wake(&wake_node->waker);

                                worker->runtime->allocator.free(worker->runtime->allocator.data, wake_node);
                                wake_node = next_wake_node;
                            }

                            if (worker->handle_id == task->id) {
                                running = false;
                            }

                            task_drop(task);
                            break;
                        }
                    }
                }

                worker->runtime->poll_scheduled = false;
            } else if (event->data.fd == worker->runtime->timer_fd) {
                uint64_t expirations = 0;
                if (read(worker->runtime->timer_fd, &expirations, sizeof(uint64_t)) != sizeof(uint64_t)) {
                    perror("timerfd read");
                }

                struct taskio_wheel_timer* wheel_timer = &worker->runtime->hierarchy_wheel.wheels[0];
                while (expirations > 0) {
                    taskio_wheel_timer_tick(wheel_timer);
                    expirations -= 1;
                }
            }
        }
    }

    return 0;
}

static void _wheel_loop(struct taskio_wheel_timer* wheel_timer) {
    struct taskio_runtime* runtime = wheel_timer->data;
    taskio_wheel_timer_tick(&runtime->hierarchy_wheel.wheels[wheel_timer->id + 1]);
}

static void _wheel_expiry(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer) {
    struct taskio_runtime* runtime = wheel_timer->data;
    taskio_runtime_add_timer_from(runtime, timer);
}

static void task_add_event_loop(struct taskio_task* task) {
    if (task->awaken) {
        return;
    }

    task->awaken = true;

    struct taskio_runtime* runtime = task->runtime;

    if (runtime->poll_tail == NULL) {
        runtime->poll_head = task;
    } else {
        runtime->poll_tail->next = task;
    }

    runtime->poll_tail = task;

    if (!runtime->poll_scheduled) {
        runtime->poll_scheduled = true;
        eventfd_write(runtime->event_fd, 1);
    }
}

static void task_wake(struct taskio_waker* waker) {
    struct taskio_task* task = waker->data;
    task_add_event_loop(task);
}

static void task_drop(struct taskio_task* task) {
    if (atomic_fetch_sub(&task->counter, 1) == 1) {
        task->runtime->allocator.free(task->runtime->allocator.data, task);
    }
}
