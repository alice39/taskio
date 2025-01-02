#include <taskio/runtime.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/unistd.h>

#include <stdlib.h>
#include <string.h>

static __thread uint64_t next_handle_id = 1;

static int worker_run(void* arg);
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

    runtime->event_fd = eventfd(0, 0);
    runtime->epoll_fd = epoll_create1(0);

    runtime->poll_head = NULL;
    runtime->poll_tail = NULL;

    struct epoll_event event = {.events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = runtime->event_fd};
    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, runtime->event_fd, &event);
}

void taskio_runtime_drop(struct taskio_runtime* runtime) {
    close(runtime->event_fd);
    close(runtime->epoll_fd);
}

struct taskio_handle taskio_runtime_spawn(struct taskio_runtime* runtime, struct taskio_future* future,
                                          size_t future_size) {
    uint64_t handle_id = next_handle_id++;

    struct taskio_task* task = malloc(sizeof(struct taskio_task));
    task->id = handle_id;
    memset(task->can_jmp, 0, sizeof(sizeof(bool[16])));
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

    eventfd_write(runtime->event_fd, 1);

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
        int nfds = epoll_wait(worker->runtime->epoll_fd, events, 1024, -1);

        for (int i = 0; i < nfds; i++) {
            struct epoll_event* event = &events[i];

            if (event->data.fd == worker->runtime->event_fd) {
                eventfd_t event_out;
                eventfd_read(worker->runtime->event_fd, &event_out);

                struct taskio_task* task = worker->runtime->poll_head;

                worker->runtime->poll_head = worker->runtime->poll_head->next;
                if (worker->runtime->poll_head == NULL) {
                    worker->runtime->poll_tail = NULL;
                }

                struct taskio_future_context context = {.waker = {
                                                            .wake = task_wake,
                                                            .worker = worker,
                                                            .task = task,
                                                            .can_jmp = task->can_jmp,
                                                            .jmp = task->jmp,
                                                            .jmp_depth = 0,
                                                        }};

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
            } else {
                // FIXME: Handle timers and other file descriptors
            }
        }
    }

    // success
    return 0;
}

static void task_wake(struct taskio_waker* waker) {
    struct taskio_worker* worker = waker->worker;
    struct taskio_runtime* runtime = worker->runtime;

    struct taskio_task* task = waker->task;

    if (runtime->poll_tail == NULL) {
        runtime->poll_head = task;
        runtime->poll_tail = task;
    } else {
        runtime->poll_tail->next = task;
        runtime->poll_tail = task;
    }

    task->can_jmp[waker->jmp_depth] = true;
    memmove(&task->jmp[waker->jmp_depth], &waker->jmp[waker->jmp_depth], sizeof(jmp_buf));

    eventfd_write(runtime->event_fd, 1);
}
