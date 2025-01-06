#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>
#include <taskio.h>

struct foo_env {
    int arg_a;
    int local_a;
};

future_fn(int, foo)(int arg_a) { return_future_fn(int, foo, arg_a); }

async_fn(int, foo) {
    async_fn_begin(int, foo);

    async_scope() {
        int arg_a = async_env(arg_a);
        int local_a = async_env(local_a) = 1;

        printf("foo: before yield: %d %d!\n", arg_a, local_a);
        yield();
    }

    async_scope() {
        int arg_a = async_env(arg_a);
        int local_a = async_env(local_a);

        printf("foo: after yield: %d %d!\n", arg_a, local_a);
        async_return(0);
    }
}

taskio_main(future_env(foo), async_env_decl(int, a)) {
    taskio_main_begin();

    async_scope() {
        async_env(a) = 1;
        printf("main: before yield: %d\n", async_env(a));
        yield();
    }

    async_scope() {
        printf("main: after yield: %d\n", async_env(a));
        await_fn(foo, 100);
    }

    async_scope() { async_return(); }
}
