#include <time.h>

#include "taskio/common.h"

static inline void taskio_sleep_poll(struct taskio_sleep_future* future,
                                     struct taskio_future_context* ctx,
                                     enum taskio_future_poll* poll,
                                     void* _out) {
    // supress unused parameter
    (void)_out;

    struct timespec current;
    struct timespec* deadline = &future->env.deadline;

    uint64_t deadline_time = deadline->tv_sec * 1000000000 + deadline->tv_nsec;

    while (1) {
        timespec_get(&current, TIME_UTC);
        uint64_t current_time = current.tv_sec * 1000000000 + current.tv_nsec;

        if (deadline_time <= current_time) {
            break;
        }

        *poll = TASKIO_FUTURE_PENDING;
        ctx->waker.wake(ctx->waker.data);
        swapcontext(future->poll_ucp, future->exec_ucp);
    }

    *poll = TASKIO_FUTURE_READY;
}

static inline void taskio_sleep_drop(struct taskio_sleep_future* future) {
    // supress unused parameter
    (void)future;
}

struct taskio_sleep_future taskio_sleep(uint64_t seconds,
                                        uint64_t nanoseconds) {
    struct timespec current;
    timespec_get(&current, TIME_UTC);

    return (struct taskio_sleep_future){
        .poll = taskio_sleep_poll,
        .drop = taskio_sleep_drop,
        .env = {
            .deadline =
                {
                    .tv_sec = current.tv_sec + seconds,
                    .tv_nsec = current.tv_nsec + nanoseconds,
                },
        }};
}
