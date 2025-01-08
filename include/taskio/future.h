#ifndef TASKIO_FUTURE_GUARD_HEADER
#define TASKIO_FUTURE_GUARD_HEADER

#include <stddef.h>

#ifdef TASKIO_TRACING
#include <stdio.h>
#define TASKIO_TRACE_UNDEFINED(future)                                                                                 \
    fprintf(stderr,                                                                                                    \
            "taskio-tracing: future '%s' at file %s:%lu is undefined.\ntaskio-tracing: check if there is missing "     \
            "yield(), "                                                                                                \
            "suspended_yield(), await*() or async_return() at the end of any async_scope in the async_fn(..., %s)\n",  \
            (future)->trace.name, (future)->trace.file, (future)->trace.line, (future)->trace.name)
#else
#define TASKIO_TRACE_UNDEFINED(future)
#endif // TASKIO_TRACING

struct taskio_waker {
    void (*wake)(struct taskio_waker* waker);
    void* data;
};

struct taskio_future_context {
    struct taskio_waker waker;

    void* runtime;
};

enum taskio_future_poll {
    taskio_future_undefined,
    taskio_future_pending,
    taskio_future_ready,
};

#ifdef TASKIO_TRACING

struct taskio_future_trace {
    const char* name;
    const char* file;
    unsigned long line;
};

#endif // TASKIO_TRACING

struct taskio_future {
#ifdef TASKIO_TRACING
    struct taskio_future_trace trace;
#endif // TASKIO_TRACING

    void (*poll)(struct taskio_future*, struct taskio_future_context*, enum taskio_future_poll*, void*);

    struct taskio_future* await_future;
    void* await_out;

    size_t counter;
};

#endif // TASKIO_FUTURE_GUARD_HEADER
