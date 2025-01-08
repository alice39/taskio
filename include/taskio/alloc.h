#ifndef TASKIO_ALLOC_GUARD_HEADER
#define TASKIO_ALLOC_GUARD_HEADER

#include <stddef.h>

struct taskio_allocator {
    void* (*alloc)(void*, size_t);
    void (*free)(void*, void*);

    void* data;
};

struct taskio_allocator taskio_default_allocator();

void* taskio_default_alloc(void* data, size_t bytes);
void taskio_default_free(void* data, void* ptr);
void* taskio_default_data();

#endif // TASKIO_ALLOC_GUARD_HEADER
