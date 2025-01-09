#ifndef TASKIO_COMMON_GUARD_HEADER
#define TASKIO_COMMON_GUARD_HEADER

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <taskio/alloc.h>
#include <taskio/async.h>

#define taskio_join(...)                                                                                               \
    taskio_join(                                                                                                       \
        &(struct taskio_allocator){                                                                                    \
            .alloc = taskio_default_alloc,                                                                             \
            .free = taskio_default_free,                                                                               \
            .data = taskio_default_data(),                                                                             \
        },                                                                                                             \
        sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

#define taskio_select(...)                                                                                             \
    (taskio_select)(                                                                                                   \
        &(struct taskio_allocator){                                                                                    \
            .alloc = taskio_default_alloc,                                                                             \
            .free = taskio_default_free,                                                                               \
            .data = taskio_default_data(),                                                                             \
        },                                                                                                             \
        true, sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

#define taskio_join_with_alloc(alloc, biased, ...)                                                                     \
    taskio_join(alloc, biased, sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

#define taskio_select_with(alloc, biased, ...)                                                                         \
    (taskio_select)(alloc, biased, sizeof((void*[]){__VA_ARGS__}) / sizeof(void*) __VA_OPT__(, ) __VA_ARGS__)

struct taskio_sleep_env {
    uint64_t ms;
    struct taskio_waker waker;

    void* timer;
};

struct taskio_join_ext_env;
struct taskio_join_task;
struct taskio_select_node;

typedef void (*taskio_join_on_ready)(struct taskio_join_ext_env* env, struct taskio_future* future, size_t index);
typedef bool (*taskio_join_on_finish)(struct taskio_join_ext_env* env, size_t* out_index);
typedef void (*taskio_join_on_cleanup)(struct taskio_join_ext_env* env);

struct taskio_join_ext_env {
    struct taskio_allocator allocator;

    size_t len;
    size_t completed_len;

    struct taskio_waker waker;

    struct taskio_join_task* head;

    struct taskio_join_task* poll_head;
    struct taskio_join_task* poll_tail;

    taskio_join_on_ready on_ready;
    taskio_join_on_finish on_finish;
    taskio_join_on_cleanup on_cleanup;
};

struct taskio_join_env {
    struct taskio_join_ext_env inner;
};

struct taskio_select_env {
    struct taskio_join_ext_env inner;

    bool biased;

    size_t len;
    struct taskio_select_node* head;
    struct taskio_select_node* tail;
};

future_fn(void, taskio_sleep)(uint64_t ms);
heap_future_fn(void, taskio_join)(struct taskio_allocator* allocator, size_t len, ...);
heap_future_fn(size_t, taskio_select)(struct taskio_allocator* allocator, bool biased, size_t len, ...);

struct taskio_join_future taskio_join_from_list(struct taskio_allocator* allocator, size_t len,
                                                struct taskio_future** futures);

#endif // TASKIO_COMMON_GUARD_HEADER
