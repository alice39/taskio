#include <stdio.h>
#include <stdlib.h>

#include <taskio.h>

struct taskio_main_env {
    int argc;
    char** args;
};

struct taskio_allocator custom_allocator();

taskio_main(.allocator = custom_allocator()) {
    async_scope() {
        printf("Hello World\n");
        async_return();
    }
}

static void* print_alloc(void* data, size_t bytes) {
    void* ptr = malloc(bytes);

    const char* data_value = data;
    printf("malloc: %lu at %p with data %p (%s)\n", bytes, ptr, data, data_value);

    return ptr;
}

static void print_free(void* data, void* ptr) {
    const char* data_value = data;
    printf("free: %p with data %p (%s)\n", ptr, data, data_value);

    free(ptr);
}

struct taskio_allocator custom_allocator() {
    return (struct taskio_allocator){.alloc = print_alloc, .free = print_free, .data = "custom-alloc"};
}
