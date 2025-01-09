#include <stdlib.h>

#include "alloc_ext.h"

static void* taskio_default_alloc([[maybe_unused]] void* data, size_t bytes) { return malloc(bytes); }

static void taskio_default_free([[maybe_unused]] void* data, void* ptr) { free(ptr); }

struct taskio_allocator taskio_default_allocator() {
    return (struct taskio_allocator){
        .alloc = taskio_default_alloc,
        .free = taskio_default_free,
        .data = NULL,
    };
}

struct taskio_allocator* taskio_default_allocator_ref() {
    static thread_local struct taskio_allocator allocator = {
        .alloc = taskio_default_alloc,
        .free = taskio_default_free,
        .data = NULL,
    };

    return &allocator;
}
