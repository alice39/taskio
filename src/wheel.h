#ifndef TASKIO_WHEEL_GUARD_HEADER
#define TASKIO_WHEEL_GUARD_HEADER

#include <stddef.h>
#include <stdint.h>

#include <taskio/alloc.h>

#include "collections/vec.h"

struct taskio_wheel_timer;
struct taskio_timer;

typedef void (*taskio_wheel_handler)(void* data);
typedef void (*taskio_wheel_loop_handler)(struct taskio_wheel_timer* wheel_timer);
typedef void (*taskio_wheel_expiry_handler)(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer,
                                            bool has_remaining_ticks);

struct taskio_timer_handle {
    uint64_t id;
    uint64_t expiry_time;
};

struct taskio_timer {
    uint64_t id;
    uint64_t expiry_time;

    taskio_wheel_handler handler;
    void* data;
};

taskio_vec_implS(taskio_timer);

struct taskio_wheel_timer {
    struct taskio_allocator* allocator;

    uint64_t tick;

    size_t id;
    uint64_t resolution;

    size_t wheel_size;
    taskio_vec(taskio_timer) * timer_buckets;

    taskio_wheel_loop_handler loop_handler;
    taskio_wheel_expiry_handler expiry_handler;
    void* data;
};

void taskio_wheel_timer_drop(struct taskio_wheel_timer* wheel_timer);

struct taskio_timer_handle taskio_wheel_timer_add(struct taskio_wheel_timer* wheel_timer, uint64_t delay,
                                                  taskio_wheel_handler handler, void* data);

void taskio_wheel_timer_add_from(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer);

void taskio_wheel_timer_tick(struct taskio_wheel_timer* wheel_timer);

bool taskio_timer_valid(struct taskio_timer_handle* handle);
void taskio_timer_fire(struct taskio_wheel_timer* wheel_timer, struct taskio_timer_handle* handle);
void taskio_timer_abort(struct taskio_wheel_timer* wheel_timer, struct taskio_timer_handle* handle);

#endif // TASKIO_WHEEL_GUARD_HEADER
