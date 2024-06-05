#ifndef TASKIO_CONTEXT_GUARD_HEADER
#define TASKIO_CONTEXT_GUARD_HEADER

#if defined(POSIX_UCONTEXT) == defined(LIBUCONTEXT_UCONTEXT)
#error                                                                         \
    "Only an ucontext library it can be used: POSIX_UCONTEXT or LIBUCONTEXT_UCONTEXT"
#elif defined(POSIX_UCONTEXT)
#include <ucontext.h>
#elif defined(LIBUCONTEXT_UCONTEXT)
#include "libucontext/libucontext.h"

#define getcontext(ctx) libucontext_getcontext(ctx)
#define makecontext(ctx, fn, len, ...)                                         \
    libucontext_makecontext(ctx, fn, len, __VA_ARGS__)
#define setcontext(ctx) libucontext_setcontext(ctx)
#define swapcontext(out_ctx, in_ctx) libucontext_swapcontext(out_ctx, in_ctx)

#endif

#endif // TASKIO_CONTEXT_GUARD_HEADER
