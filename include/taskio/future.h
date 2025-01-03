#ifndef TASKIO_FUTURE_GUARD_HEADER
#define TASKIO_FUTURE_GUARD_HEADER

#include <setjmp.h>
#include <stdbool.h>

struct taskio_waker {
    void (*wake)(struct taskio_waker* waker);

    void* worker;
    void* task;

    int jmp_depth;
    bool* can_jmp;
    jmp_buf* jmp;
};

struct taskio_future_context {
    struct taskio_waker waker;
};

enum taskio_future_poll {
    TASKIO_FUTURE_READY,
    TASKIO_FUTURE_PENDING,
};

struct taskio_future {
    void (*poll)(volatile struct taskio_future* volatile future, volatile struct taskio_future_context* volatile ctx,
                 volatile enum taskio_future_poll* volatile poll, volatile void* volatile value);
};

#endif // TASKIO_FUTURE_GUARD_HEADER
