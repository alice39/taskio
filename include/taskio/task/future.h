#ifndef TASKIO_FUTURE_GUARD_HEADER
#define TASKIO_FUTURE_GUARD_HEADER

#include "../context.h"

struct taskio_waker {
    struct taskio_waker (*clone)(void* data);
    void (*wake)(void* data);
    void (*wake_by_ref)(void* data);
    void (*drop)(void* data);

    void* data;
};

struct taskio_future_context {
    struct taskio_waker waker;
};

enum taskio_future_poll {
    TASKIO_FUTURE_READY,
    TASKIO_FUTURE_PENDING,
};

struct taskio_future {
    void (*poll)(void* future, struct taskio_future_context* ctx,
                 enum taskio_future_poll* poll, void* value);
    void (*drop)(void* future);

    ucontext_t* exec_ucp;
    ucontext_t* poll_ucp;
    ucontext_t* drop_ucp;
};

#endif // TASKIO_FUTURE_GUARD_HEADER
