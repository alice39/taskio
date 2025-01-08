#include <stdlib.h>

#include <taskio/alloc.h>

struct taskio_allocator taskio_default_allocator() {
    return (struct taskio_allocator){
        .alloc = taskio_default_alloc,
        .free = taskio_default_free,
        .data = taskio_default_data(),
    };
}

void* taskio_default_alloc([[maybe_unused]] void* data, size_t bytes) { return malloc(bytes); }

void taskio_default_free([[maybe_unused]] void* data, void* ptr) { free(ptr); }

void* taskio_default_data() { return NULL; }
