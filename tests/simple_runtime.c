#define TASKIO_RUNTIME SIMPLE

#include <stdio.h>
#include <taskio.h>

struct foo_env {
    int a;
    int local_a;
};

future_fn(int, foo)(int a) { return_future_fn(int, foo, a); }

async_fn(
    int, foo,
    {
        local_a = 1;
        printf("foo: before yield: %d %d!\n", a, local_a);
        yield();
        printf("foo: after yield: %d %d!\n", a, local_a);

        async_return(0);
    },
    async_scope(int, a), async_scope(int, local_a));

taskio_main(
    {
        a = 1;
        printf("main: before yield: %d\n", a);
        yield();
        printf("main: after yield: %d\n", a);

        await_fn(foo, 100);
    },
    future_env(foo), async_scope(int, a));
