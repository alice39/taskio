#ifndef TASKIO_ALLOC_GUARD_HEADER
#define TASKIO_ALLOC_GUARD_HEADER

#include <stddef.h>

struct taskio_allocator {
    void* (*alloc)(size_t);
    void (*free)(void*);
};

#endif // TASKIO_ALLOC_GUARD_HEADER
