#ifndef TASKIO_ALLOC_GUARD_HEADER
#define TASKIO_ALLOC_GUARD_HEADER

#include <stddef.h>

struct taskio_allocator {
    void* (*alloc)(void*, size_t);
    void (*free)(void*, void*);

    void* data;
};

void taskio_initialize_allocator(struct taskio_allocator* allocator);
struct taskio_allocator* taskio_default_allocator();

#endif // TASKIO_ALLOC_GUARD_HEADER
