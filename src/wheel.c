#include <stdlib.h>

#include "wheel.h"

void taskio_wheel_timer_init(struct taskio_wheel_timer* wheel_timer, size_t id, void* data, size_t len,
                             taskio_wheel_loop_handler loop_handler, taskio_wheel_expiry_handler expiry_handler) {
    wheel_timer->id = id;
    wheel_timer->data = data;
    wheel_timer->tick = 0;

    wheel_timer->len = len;
    wheel_timer->timer_buckets = calloc(len, sizeof(struct taskio_timer*));

    wheel_timer->loop_handler = loop_handler;
    wheel_timer->expiry_handler = expiry_handler;
}

void taskio_wheel_timer_drop(struct taskio_wheel_timer* wheel_timer) {
    for (size_t i = 0; i < wheel_timer->len; i++) {
        struct taskio_timer* timer = wheel_timer->timer_buckets[i];
        while (timer) {
            struct taskio_timer* next_timer = timer->next;
            free(timer);
            timer = next_timer;
        }
    }

    wheel_timer->id = 0;
    wheel_timer->tick = 0;
    wheel_timer->len = 0;
    free(wheel_timer->timer_buckets);
    wheel_timer->timer_buckets = NULL;
    wheel_timer->loop_handler = NULL;
}

void taskio_wheel_timer_add(struct taskio_wheel_timer* wheel_timer, uint64_t delay, taskio_wheel_handler handler,
                            void* data) {
    struct taskio_timer* timer = malloc(sizeof(struct taskio_timer));
    timer->remaining = delay;
    timer->handler = handler;
    timer->data = data;
    timer->next = NULL;

    taskio_wheel_timer_add_from(wheel_timer, timer);
}

void taskio_wheel_timer_add_from(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer) {
    uint64_t delay = timer->remaining;
    if (delay == 0) {
        delay = 1;
    }

    if (delay > wheel_timer->len) {
        timer->remaining = delay - wheel_timer->len;
    } else {
        timer->remaining = 0;
    }

    size_t index = (wheel_timer->tick + delay) % wheel_timer->len;
    timer->next = wheel_timer->timer_buckets[index];
    wheel_timer->timer_buckets[index] = timer;
}

void taskio_wheel_timer_tick(struct taskio_wheel_timer* wheel_timer) {
    size_t tick = wheel_timer->tick + 1;
    size_t index = tick % wheel_timer->len;

    struct taskio_timer* timer = wheel_timer->timer_buckets[index];
    struct taskio_timer* expired_timer_head = NULL;

    wheel_timer->timer_buckets[index] = NULL;

    while (timer != NULL) {
        struct taskio_timer* next_timer = timer->next;
        timer->next = NULL;

        if (timer->remaining == 0) {
            timer->handler(timer->data);
            free(timer);
        } else if (wheel_timer->expiry_handler) {
            wheel_timer->expiry_handler(wheel_timer, timer);
        } else {
            timer->next = expired_timer_head;
            expired_timer_head = timer;
        }

        timer = next_timer;
    }

    while (expired_timer_head != NULL) {
        struct taskio_timer* next_expired_timer = expired_timer_head->next;

        expired_timer_head->next = NULL;
        taskio_wheel_timer_add_from(wheel_timer, expired_timer_head);

        expired_timer_head = next_expired_timer;
    }

    if (wheel_timer->loop_handler && tick > 0 && index == 0) {
        wheel_timer->loop_handler(wheel_timer);
    }

    wheel_timer->tick += 1;
}
