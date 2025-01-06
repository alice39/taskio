#include <stdlib.h>

#include "../../runtime_ext.h"
#include "platform_runtime.h"

struct taskio_timer* taskio_runtime_add_timer(struct taskio_runtime* runtime, uint64_t delay,
                                              taskio_wheel_handler handler, void* data) {
    size_t ten_ms_index = delay / 10;
    size_t hundred_ms_index = delay / 100;
    size_t second_index = delay / 1000;
    size_t minute_index = delay / (1000 * 60);
    size_t hour_index = delay / (1000 * 60 * 60);
    size_t day_index = delay / (1000 * 60 * 60 * 24);
    size_t year_index = delay / (1000 * 60 * 60 * 24 * 365UL);

    struct taskio_wheel_timer* wheel_timer = NULL;

    if (year_index > 0) {
        wheel_timer = &runtime->platform->wheels[7];
    } else if (day_index > 0) {
        wheel_timer = &runtime->platform->wheels[6];
    } else if (hour_index > 0) {
        wheel_timer = &runtime->platform->wheels[5];
    } else if (minute_index > 0) {
        wheel_timer = &runtime->platform->wheels[4];
    } else if (second_index > 0) {
        wheel_timer = &runtime->platform->wheels[3];
    } else if (hundred_ms_index > 0) {
        wheel_timer = &runtime->platform->wheels[2];
    } else if (ten_ms_index > 0) {
        wheel_timer = &runtime->platform->wheels[1];
    } else {
        wheel_timer = &runtime->platform->wheels[0];
    }

    return taskio_wheel_timer_add(wheel_timer, delay, handler, data);
}

void taskio_runtime_add_timer_from(struct taskio_runtime* runtime, struct taskio_timer* timer) {
    uint64_t time = runtime->platform->wheels[0].tick * runtime->platform->wheels[0].resolution;
    if (time >= timer->expiry_time) {
        timer->handler(timer->data);
        free(timer);
        return;
    }

    uint64_t delay = timer->expiry_time - time;

    size_t ten_ms_index = delay / 10;
    size_t hundred_ms_index = delay / 100;
    size_t second_index = delay / 1000;
    size_t minute_index = delay / (1000 * 60);
    size_t hour_index = delay / (1000 * 60 * 60);
    size_t day_index = delay / (1000 * 60 * 60 * 24);
    size_t year_index = delay / (1000 * 60 * 60 * 24 * 365UL);

    struct taskio_wheel_timer* wheel_timer = NULL;

    if (year_index > 0) {
        wheel_timer = &runtime->platform->wheels[7];
    } else if (day_index > 0) {
        wheel_timer = &runtime->platform->wheels[6];
    } else if (hour_index > 0) {
        wheel_timer = &runtime->platform->wheels[5];
    } else if (minute_index > 0) {
        wheel_timer = &runtime->platform->wheels[4];
    } else if (second_index > 0) {
        wheel_timer = &runtime->platform->wheels[3];
    } else if (hundred_ms_index > 0) {
        wheel_timer = &runtime->platform->wheels[2];
    } else if (ten_ms_index > 0) {
        wheel_timer = &runtime->platform->wheels[1];
    } else {
        wheel_timer = &runtime->platform->wheels[0];
    }

    taskio_wheel_timer_add_from(wheel_timer, timer);
}
