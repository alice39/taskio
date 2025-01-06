#include <stdlib.h>

#include "wheel.h"

void taskio_wheel_timer_init(struct taskio_wheel_timer* wheel_timer, size_t id, uint64_t resolution, size_t len,
                             taskio_wheel_loop_handler loop_handler, taskio_wheel_expiry_handler expiry_handler,
                             void* data) {
    wheel_timer->tick = 0;
    wheel_timer->id = id;

    wheel_timer->resolution = resolution;

    wheel_timer->wheel_size = len;
    wheel_timer->timer_buckets = calloc(len, sizeof(struct taskio_timer*));

    wheel_timer->loop_handler = loop_handler;
    wheel_timer->expiry_handler = expiry_handler;
    wheel_timer->data = data;
}

void taskio_wheel_timer_drop(struct taskio_wheel_timer* wheel_timer) {
    for (size_t i = 0; i < wheel_timer->wheel_size; i++) {
        struct taskio_timer* timer = wheel_timer->timer_buckets[i];
        while (timer) {
            struct taskio_timer* next_timer = timer->next;
            free(timer);
            timer = next_timer;
        }
    }

    wheel_timer->tick = 0;
    wheel_timer->id = 0;

    wheel_timer->resolution = 0;

    wheel_timer->wheel_size = 0;
    free(wheel_timer->timer_buckets);

    wheel_timer->loop_handler = NULL;
    wheel_timer->expiry_handler = NULL;
    wheel_timer->data = NULL;
}

struct taskio_timer* taskio_wheel_timer_add(struct taskio_wheel_timer* wheel_timer, uint64_t delay,
                                            taskio_wheel_handler handler, void* data) {
    if (delay == 0) {
        handler(data);
        return NULL;
    }

    struct taskio_timer* timer = malloc(sizeof(struct taskio_timer));
    // timer is handled by the user and the wheel
    timer->counter = 2;
    timer->expiry_time = wheel_timer->tick * wheel_timer->resolution + delay;
    timer->handler = handler;
    timer->data = data;
    timer->next = NULL;

    taskio_wheel_timer_add_from(wheel_timer, timer);
    return timer;
}

void taskio_wheel_timer_add_from(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer) {
    uint64_t time = wheel_timer->tick * wheel_timer->resolution;
    if (time >= timer->expiry_time) {
        // fired
        timer->bucket = NULL;
        timer->handler(timer->data);

        taskio_timer_drop(timer);
        return;
    }

    uint64_t remaining_ticks = (timer->expiry_time - time) / wheel_timer->resolution;
    size_t bucket_index = (wheel_timer->tick + remaining_ticks) % wheel_timer->wheel_size;

    timer->bucket = &wheel_timer->timer_buckets[bucket_index];

    timer->next = wheel_timer->timer_buckets[bucket_index];
    wheel_timer->timer_buckets[bucket_index] = timer;
}

void taskio_wheel_timer_tick(struct taskio_wheel_timer* wheel_timer) {
    uint64_t tick = wheel_timer->tick + 1;
    uint64_t time = tick * wheel_timer->resolution;

    size_t index = tick % wheel_timer->wheel_size;

    struct taskio_timer* timer = wheel_timer->timer_buckets[index];
    struct taskio_timer* expired_timer_head = NULL;

    wheel_timer->timer_buckets[index] = NULL;

    while (timer != NULL) {
        struct taskio_timer* next_timer = timer->next;
        timer->next = NULL;

        if (time >= timer->expiry_time) {
            // fired
            timer->bucket = NULL;
            timer->handler(timer->data);

            taskio_timer_drop(timer);
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

void taskio_timer_clone(struct taskio_timer* timer) { timer->counter++; }

void taskio_timer_drop(struct taskio_timer* timer) {
    if (atomic_fetch_sub(&timer->counter, 1) == 1) {
        free(timer);
    }
}

void taskio_timer_fire(struct taskio_timer* timer) {
    struct taskio_timer** bucket = timer->bucket;
    timer->bucket = NULL;

    if (bucket) {
        timer->handler(timer->data);

        struct taskio_timer* current = *bucket;
        struct taskio_timer* previous = NULL;

        while (current && current != timer) {
            previous = current;
            current = current->next;
        }

        if (previous) {
            previous->next = timer->next;
        } else {
            *bucket = timer->next;
        }

        // dropped for the wheel
        taskio_timer_drop(timer);
    }

    // dropped for the user
    taskio_timer_drop(timer);
}

void taskio_timer_abort(struct taskio_timer* timer) {
    struct taskio_timer** bucket = timer->bucket;
    timer->bucket = NULL;

    if (bucket) {
        struct taskio_timer* current = *bucket;
        struct taskio_timer* previous = NULL;

        while (current && current != timer) {
            previous = current;
            current = current->next;
        }

        if (previous) {
            previous->next = timer->next;
        } else {
            *bucket = timer->next;
        }

        // dropped for the wheel
        taskio_timer_drop(timer);
    }

    // dropped for the user
    taskio_timer_drop(timer);
}
