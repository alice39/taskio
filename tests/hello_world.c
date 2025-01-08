#include <stdio.h>

#include <taskio.h>

struct taskio_main_env {
    int argc;
    char** args;
};

taskio_main() {
    async_scope() {
        printf("Hello World\n");
        async_return();
    }
}
