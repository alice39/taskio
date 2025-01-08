#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>

struct taskio_main_env {
    int argc;
    char** args;
};

void* print_alloc(size_t bytes) {
    void* ptr = malloc(bytes);
    printf("malloc: %lu at %p\n", bytes, ptr);
    return ptr;
}

void print_free(void* ptr) {
    printf("free: %p\n", ptr);
    free(ptr);
}

taskio_main(print_alloc, print_free) {
    taskio_main_begin();

    async_scope() {
        printf("Hello World\n");
        async_return();
    }
}
