#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>

#include <taskio.h>

struct taskio_main_env {
    int argc;
    char** args;
};

taskio_main() {
    taskio_main_begin();

    async_scope() {
        printf("Hello World\n");
        async_return();
    }
}
