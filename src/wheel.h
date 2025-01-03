#ifndef TASKIO_WHEEL_GUARD_HEADER
#define TASKIO_WHEEL_GUARD_HEADER

#include <stddef.h>
#include <stdint.h>

struct taskio_wheel_timer;
struct taskio_timer;

typedef void (*taskio_wheel_handler)(void* data);
typedef void (*taskio_wheel_loop_handler)(struct taskio_wheel_timer*);
typedef void (*taskio_wheel_expiry_handler)(struct taskio_wheel_timer*, struct taskio_timer*);

struct taskio_timer {
    uint64_t remaining;

    taskio_wheel_handler handler;
    void* data;

    struct taskio_timer* next;
};

struct taskio_wheel_timer {
    size_t id;
    void* data;
    size_t tick;

    size_t len;
    struct taskio_timer** timer_buckets;

    taskio_wheel_loop_handler loop_handler;
    taskio_wheel_expiry_handler expiry_handler;
};

void taskio_wheel_timer_init(struct taskio_wheel_timer* wheel_timer, size_t id, void* data, size_t len,
                             taskio_wheel_loop_handler loop_handler, taskio_wheel_expiry_handler expiry_handler);
void taskio_wheel_timer_drop(struct taskio_wheel_timer* wheel_timer);

void taskio_wheel_timer_add(struct taskio_wheel_timer* wheel_timer, uint64_t delay, taskio_wheel_handler handler,
                            void* data);
void taskio_wheel_timer_add_from(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer);

void taskio_wheel_timer_tick(struct taskio_wheel_timer* wheel_timer);

#endif // TASKIO_WHEEL_GUARD_HEADER
