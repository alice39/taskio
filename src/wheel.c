#include <stdatomic.h>

#include "collections/vec.h"
#include "wheel.h"

static atomic_uint_fast64_t next_timer_id;

static taskio_vec(taskio_timer) * _find_bucket(struct taskio_wheel_timer* wheel_timer, uint64_t expiry_time);

static enum taskio_predicate _fire_predicate(struct taskio_timer* timer, void* data);
static enum taskio_predicate _abort_predicate(struct taskio_timer* timer, void* data);

void taskio_wheel_timer_drop(struct taskio_wheel_timer* wheel_timer) {
    struct taskio_allocator* allocator = wheel_timer->allocator;

    for (size_t i = 0; i < wheel_timer->wheel_size; i++) {
        taskio_vec(taskio_timer)* timers = &wheel_timer->timer_buckets[i];
        taskio_vec_destroy(timers, allocator);
    }

    wheel_timer->tick = 0;
    wheel_timer->id = 0;

    wheel_timer->resolution = 0;

    wheel_timer->wheel_size = 0;
    wheel_timer->timer_buckets = NULL;

    wheel_timer->loop_handler = NULL;
    wheel_timer->expiry_handler = NULL;
    wheel_timer->data = NULL;
}

struct taskio_timer_handle taskio_wheel_timer_add(struct taskio_wheel_timer* wheel_timer, uint64_t delay,
                                                  taskio_wheel_handler handler, void* data) {
    if (delay == 0) {
        handler(data);
        return (struct taskio_timer_handle){};
    }

    struct taskio_timer timer = {
        .id = atomic_fetch_add(&next_timer_id, 1),
        .expiry_time = wheel_timer->tick * wheel_timer->resolution + delay,
        .handler = handler,
        .data = data,
    };

    taskio_wheel_timer_add_from(wheel_timer, &timer);

    return (struct taskio_timer_handle){
        .id = timer.id,
        .expiry_time = timer.expiry_time,
    };
}

void taskio_wheel_timer_add_from(struct taskio_wheel_timer* wheel_timer, struct taskio_timer* timer) {
    taskio_vec(taskio_timer)* timers = _find_bucket(wheel_timer, timer->expiry_time);

    if (timers) {
        taskio_vec_push_last(timers, wheel_timer->allocator, *timer);
    } else {
        timer->handler(timer->data);
    }
}

void taskio_wheel_timer_tick(struct taskio_wheel_timer* wheel_timer) {
    uint64_t tick = wheel_timer->tick + 1;
    uint64_t time = tick * wheel_timer->resolution;

    size_t index = tick % wheel_timer->wheel_size;

    taskio_vec(taskio_timer) timers;
    taskio_vec_move(&timers, &wheel_timer->timer_buckets[index]);

    for (size_t i = 0; i < taskio_vec_len(&timers); i++) {
        struct taskio_timer* timer = &taskio_vec_at(&timers, i);

        if (time >= timer->expiry_time) {
            timer->handler(timer->data);
            wheel_timer->expiry_handler(wheel_timer, timer, false);
        } else if (wheel_timer->expiry_handler) {
            wheel_timer->expiry_handler(wheel_timer, timer, true);
        } else {
            taskio_wheel_timer_add_from(wheel_timer, timer);
        }
    }

    taskio_vec_destroy(&timers, wheel_timer->allocator);

    if (wheel_timer->loop_handler && tick > 0 && index == 0) {
        wheel_timer->loop_handler(wheel_timer);
    }

    wheel_timer->tick += 1;
}

bool taskio_timer_valid(struct taskio_timer_handle* handle) { return handle->id != 0 || handle->expiry_time != 0; }

void taskio_timer_fire(struct taskio_wheel_timer* wheel_timer, struct taskio_timer_handle* handle) {
    taskio_vec(taskio_timer)* timers = _find_bucket(wheel_timer, handle->expiry_time);
    if (timers == NULL) {
        return;
    }

    taskio_vec_remove_if(timers, wheel_timer->allocator, _fire_predicate, handle);
}

void taskio_timer_abort(struct taskio_wheel_timer* wheel_timer, struct taskio_timer_handle* handle) {
    taskio_vec(taskio_timer)* timers = _find_bucket(wheel_timer, handle->expiry_time);
    if (timers == NULL) {
        return;
    }

    taskio_vec_remove_if(timers, wheel_timer->allocator, _abort_predicate, handle);
}

static taskio_vec(taskio_timer) * _find_bucket(struct taskio_wheel_timer* wheel_timer, uint64_t expiry_time) {
    uint64_t time = wheel_timer->tick * wheel_timer->resolution;
    if (time >= expiry_time) {
        return NULL;
    }

    uint64_t remaining_ticks = (expiry_time - time) / wheel_timer->resolution;
    size_t bucket_index = (wheel_timer->tick + remaining_ticks) % wheel_timer->wheel_size;

    return &wheel_timer->timer_buckets[bucket_index];
}

static enum taskio_predicate _fire_predicate(struct taskio_timer* timer, void* data) {
    struct taskio_timer_handle* handle = data;
    if (timer->id == handle->id) {
        timer->handler(timer->data);
        return taskio_predicate_true_finish;
    } else {
        return taskio_predicate_false;
    }
}

static enum taskio_predicate _abort_predicate(struct taskio_timer* timer, void* data) {
    struct taskio_timer_handle* handle = data;
    if (timer->id == handle->id) {
        return taskio_predicate_true_finish;
    } else {
        return taskio_predicate_false;
    }
}
