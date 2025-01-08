#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>
#include <stdlib.h>

#include <taskio.h>

struct taskio_main_env {
    int argc;
    char** args;
};

void* print_alloc(void* data, size_t bytes) {
    void* ptr = malloc(bytes);

    const char* data_value = data;
    printf("malloc: %lu at %p with data %p (%s)\n", bytes, ptr, data, data_value);

    return ptr;
}

void print_free(void* data, void* ptr) {
    const char* data_value = data;
    printf("free: %p with data %p (%s)\n", ptr, data, data_value);

    free(ptr);
}

struct taskio_allocator with_allocator() {
    return (struct taskio_allocator){.alloc = print_alloc, .free = print_free, .data = "custom-alloc"};
}

taskio_main(with_allocator()) {
    taskio_main_begin();

    async_scope() {
        printf("Hello World\n");
        async_return();
    }
}
