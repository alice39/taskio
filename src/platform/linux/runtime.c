#include <stdio.h>
#include <string.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/unistd.h>

#include <taskio/async.h>
#include <taskio/runtime.h>

#include "runtime_platform.h"

static void _wheel_setup(struct taskio_runtime*);
static int worker_run(void* arg);
static int blocking_worker_run(void* arg);

static void _wheel_loop(struct taskio_wheel_timer* wheel_timer);
static void _wheel_expiry(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer, bool has_remaining_ticks);

static void task_add_event_loop(struct taskio_task* task);
static void task_wake(struct taskio_waker* waker);

static void task_wake_wakers(struct taskio_task_base* task);

static void task_drop(struct taskio_task_base* task);

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

        worker->task = NULL;
        worker->task_out = NULL;
    }

    sem_init(&runtime->blocking_sem, 0, 0);

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_init(&runtime->poll_guard, mtx_plain);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_init(&runtime->blocking_guard, mtx_plain);

    runtime->event_fd = eventfd(0, 0);
    runtime->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    runtime->epoll_fd = epoll_create1(0);

    _wheel_setup(runtime);

    timerfd_settime(runtime->timer_fd, 0,
                    &(struct itimerspec){
                        .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 0},
                        .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 0},
                    },
                    NULL);

    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, runtime->event_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->event_fd});

    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, runtime->timer_fd,
              &(struct epoll_event){.events = EPOLLIN, .data.fd = runtime->timer_fd});

    runtime->poll_scheduled = false;
    runtime->poll_head = NULL;
    runtime->poll_tail = NULL;
}

void taskio_runtime_drop(struct taskio_runtime* runtime) {
    close(runtime->event_fd);
    close(runtime->timer_fd);
    close(runtime->epoll_fd);

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_destroy(&runtime->poll_guard);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_destroy(&runtime->blocking_guard);

    sem_destroy(&runtime->blocking_sem);

    for (size_t i = 0; i < 8; i++) {
        taskio_wheel_timer_drop(&runtime->hierarchy_wheel.wheels[i]);
    }
}

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size, size_t out_size) {
    struct taskio_task* task =
        runtime->allocator.alloc(runtime->allocator.data, sizeof(struct taskio_task) + future_size + out_size);

    // handled by the user and runtime
    task->base.counter = 2;

    task->base.blocking = false;
    task->base.status = taskio_task_suspended;

    task->base.runtime = runtime;
    task->base.wake_on_ready_top = NULL;

    task->base.out_size = out_size;
    if (out_size == 0) {
        task->base.out = NULL;
    } else {
        task->base.out = (void*)&task->raw_futures[future_size];
    }

    task->next = NULL;

    if (future_size == 0) {
        task->future = future;
    } else {
        task->future = (struct taskio_future*)&task->raw_futures[0];
        memcpy(task->future, future, future_size);
    }

    task_add_event_loop(task);

    return (struct taskio_handle){
        .task = &task->base,
    };
}

struct taskio_handle taskio_runtime_spawn_blocking(struct taskio_runtime* runtime,
                                                   void (*handler)(void* data, void* out), void* data,
                                                   size_t out_size) {
    struct taskio_blocking_task* task =
        runtime->allocator.alloc(runtime->allocator.data, sizeof(struct taskio_blocking_task) + out_size);

    // handled by the user and runtime
    task->base.counter = 2;

    task->base.blocking = true;
    task->base.status = taskio_task_suspended;

    task->base.runtime = runtime;
    task->base.wake_on_ready_top = NULL;

    task->base.out_size = out_size;
    if (out_size == 0) {
        task->base.out = NULL;
    } else {
        task->base.out = (void*)&task->raw_out[0];
    }

    task->next = NULL;

    task->handler = handler;
    task->data = data;

    mtx_lock(&runtime->blocking_guard);
    if (runtime->blocking_tail == NULL) {
        runtime->blocking_head = task;
    } else {
        runtime->blocking_tail->next = task;
    }

    runtime->blocking_tail = task;

    sem_post(&runtime->blocking_sem);
    mtx_unlock(&runtime->blocking_guard);

    if (!runtime->blocking_initialized) {
        runtime->blocking_initialized = true;

        for (size_t i = 0; i < BLOCKING_WORKER_SIZE; i++) {
            thrd_create(&runtime->blocking_workers[i].id, blocking_worker_run, runtime);
        }
    }

    return (struct taskio_handle){
        .task = &task->base,
    };
}

void taskio_runtime_block_on(struct taskio_runtime* runtime, struct taskio_future* future, void* out) {
    struct taskio_handle handle = taskio_runtime_spawn(runtime, future, 0, 0);
    struct taskio_worker worker = {.runtime = runtime, .task = handle.task, .task_out = out};

    taskio_handle_drop(&handle);
    worker_run(&worker);
}

struct taskio_handle taskio_handle_clone(struct taskio_handle* handle) {
    struct taskio_task_base* task = handle->task;
    task->counter += 1;

    return (struct taskio_handle){
        .task = handle->task,
    };
}

void taskio_handle_drop(struct taskio_handle* handle) {
    struct taskio_task_base* task = handle->task;
    task_drop(task);

    handle->task = NULL;
}

bool taskio_handle_is_aborted(struct taskio_handle* handle) {
    struct taskio_task_base* task = handle->task;
    return task->status == taskio_task_aborted;
}

bool taskio_handle_is_finished(struct taskio_handle* handle) {
    struct taskio_task_base* task = handle->task;
    return task->status == taskio_task_finished;
}

void taskio_handle_abort(struct taskio_handle* handle) {
    struct taskio_task_base* task = handle->task;
    switch (task->status) {
        case taskio_task_suspended: {
            task->status = taskio_task_aborted;

            if (!task->blocking) {
                task_add_event_loop((struct taskio_task*)task);
            }
            break;
        }
        case taskio_task_scheduled: {
            if (!task->blocking) {
                task->status = taskio_task_aborted;
            }
        }
        case taskio_task_aborted:
        case taskio_task_finished: {
            // nothing to do
            break;
        }
    }
}

void taskio_handle_join(struct taskio_handle* handle, struct taskio_waker* waker, void* out) {
    struct taskio_task_base* task = handle->task;
    switch (task->status) {
        case taskio_task_suspended:
        case taskio_task_scheduled: {
            break;
        }
        case taskio_task_aborted: {
            waker->wake(waker);
            break;
        }
        case taskio_task_finished: {
            if (out != NULL && task->out != NULL) {
                memcpy(out, task->out, task->out_size);
            }
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
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
                    mtx_lock(&worker->runtime->poll_guard);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

                    struct taskio_task* task = worker->runtime->poll_head;
                    if (task == NULL) {
                        break;
                    }

                    worker->runtime->poll_head = worker->runtime->poll_head->next;
                    if (worker->runtime->poll_head == NULL) {
                        worker->runtime->poll_tail = NULL;
                    }

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
                    mtx_unlock(&worker->runtime->poll_guard);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

                    if (task->base.status == taskio_task_aborted) {
                        __TASKIO_FUTURE_CLEANUP(task->future);

                        if (worker->task == task) {
                            running = false;
                        }

                        task_drop(&task->base);
                        continue;
                    }

                    task->base.status = taskio_task_suspended;
                    task->next = NULL;

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
                    void* out = worker->task == task ? worker->task_out : task->base.out;
                    task->future->poll(task->future, &context, &poll, out);

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
                            __TASKIO_FUTURE_CLEANUP(task->future);

                            task->base.status = taskio_task_finished;

                            task_wake_wakers(&task->base);

                            if (worker->task == task) {
                                running = false;
                            }

                            task_drop(&task->base);
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

static int blocking_worker_run(void* arg) {
    struct taskio_runtime* runtime = arg;

    while (true) {
        sem_wait(&runtime->blocking_sem);
        mtx_lock(&runtime->blocking_guard);

        struct taskio_blocking_task* task = runtime->blocking_head;
        runtime->blocking_head = task->next;
        if (runtime->blocking_head == NULL) {
            runtime->blocking_tail = NULL;
        }

        mtx_unlock(&runtime->blocking_guard);

        if (task->base.status != taskio_task_aborted) {
            task->handler(task->data, task->base.out);
            task->base.status = taskio_task_finished;

            task_wake_wakers(&task->base);
        }

        task_drop(&task->base);
    }

    return 0;
}

static void _wheel_setup(struct taskio_runtime* runtime) {
    struct taskio_wheel_timer* wheels = runtime->hierarchy_wheel.wheels;
    taskio_vec(taskio_timer)* buckets = runtime->hierarchy_wheel.buckets;

    runtime->hierarchy_wheel.timer_len = 0;

    uint64_t resolutions[WHEEL_LEVEL_SIZE] = {1, 10, 100, 1000, 60000, 3600000, 86400000, 31536000000};
    size_t lengths[WHEEL_LEVEL_SIZE] = {10, 10, 10, 60, 60, 24, 365, 4};

    size_t buckets_start = 0;

    for (size_t i = 0; i < WHEEL_LEVEL_SIZE; i++) {
        wheels[i].allocator = &runtime->allocator;

        wheels[i].tick = 0;
        wheels[i].id = i;

        wheels[i].resolution = resolutions[i];

        wheels[i].wheel_size = lengths[i];
        wheels[i].timer_buckets = buckets + buckets_start;
        buckets_start += lengths[i];

        wheels[i].loop_handler = _wheel_loop;
        wheels[i].expiry_handler = _wheel_expiry;
        wheels[i].data = runtime;
    }
}

static void _wheel_loop(struct taskio_wheel_timer* wheel_timer) {
    struct taskio_runtime* runtime = wheel_timer->data;
    taskio_wheel_timer_tick(&runtime->hierarchy_wheel.wheels[wheel_timer->id + 1]);
}

static void _wheel_expiry(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer,
                          bool has_remaining_ticks) {
    struct taskio_runtime* runtime = wheel_timer->data;
    if (has_remaining_ticks) {
        taskio_runtime_add_timer_from(runtime, timer, true);
    } else {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
        size_t timer_len = atomic_fetch_sub(&runtime->hierarchy_wheel.timer_len, 1);
#else
        size_t timer_len = runtime->hierarchy_wheel.timer_len--;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

        // disarms the timer when there is no timers in the wheels
        if (timer_len == 1) {
            timerfd_settime(runtime->timer_fd, 0,
                            &(struct itimerspec){
                                .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 0},
                                .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 0},
                            },
                            NULL);
        }
    }
}

static void task_add_event_loop(struct taskio_task* task) {
    if (task->base.status == taskio_task_scheduled) {
        return;
    }

    task->base.status = taskio_task_scheduled;

    struct taskio_runtime* runtime = task->base.runtime;

#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_lock(&worker->runtime->poll_guard);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE
    if (runtime->poll_tail == NULL) {
        runtime->poll_head = task;
    } else {
        runtime->poll_tail->next = task;
    }

    runtime->poll_tail = task;
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    mtx_unlock(&worker->runtime->poll_guard);
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    if (!runtime->poll_scheduled) {
        runtime->poll_scheduled = true;
        eventfd_write(runtime->event_fd, 1);
    }
}

static void task_wake(struct taskio_waker* waker) {
    struct taskio_task* task = waker->data;
    task_add_event_loop(task);
}

static void task_wake_wakers(struct taskio_task_base* task) {
    struct taskio_task_wake_node* wake_node = task->wake_on_ready_top;
    while (wake_node) {
        struct taskio_task_wake_node* next_wake_node = wake_node->next;

        if (wake_node->out) {
            memcpy(wake_node->out, task->out, task->out_size);
        }
        wake_node->waker.wake(&wake_node->waker);

        task->runtime->allocator.free(task->runtime->allocator.data, wake_node);
        wake_node = next_wake_node;
    }
}

static void task_drop(struct taskio_task_base* task) {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
    size_t counter = atomic_fetch_sub(&task->base.counter, 1);
#else
    size_t counter = task->counter--;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

    if (counter == 1) {
        task->runtime->allocator.free(task->runtime->allocator.data, task);
    }
}
