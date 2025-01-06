#ifndef TASKIO_FUTURE_GUARD_HEADER
#define TASKIO_FUTURE_GUARD_HEADER

#include <stddef.h>

struct taskio_waker {
    void (*wake)(struct taskio_waker* waker);
    void* data;
};

struct taskio_future_context {
    struct taskio_waker waker;

    void* worker;
};

enum taskio_future_poll {
    TASKIO_FUTURE_READY,
    TASKIO_FUTURE_PENDING,
};

struct taskio_future {
    void (*poll)(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);

    struct taskio_future* await_future;
    void* await_out;

    size_t counter;
};

#endif // TASKIO_FUTURE_GUARD_HEADER
