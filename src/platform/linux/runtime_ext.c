#include <assert.h>

#include <sys/timerfd.h>

#include "../../runtime_ext.h"
#include "runtime_platform.h"

static inline struct taskio_wheel_timer* _find_wheel(struct taskio_runtime* runtime, uint64_t delay);

struct taskio_timer* taskio_runtime_add_timer(struct taskio_runtime* runtime, uint64_t delay,
                                              taskio_wheel_handler handler, void* data) {
    struct taskio_wheel_timer* wheel_timer = _find_wheel(runtime, delay);
    struct taskio_timer* timer = taskio_wheel_timer_add(wheel_timer, delay, handler, data);

    if (timer) {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
        size_t timer_len = atomic_fetch_add(&runtime->hierarchy_wheel.timer_len, 1);
#else
        size_t timer_len = runtime->hierarchy_wheel.timer_len++;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

        if (timer_len == 0) {
            timerfd_settime(runtime->timer_fd, 0,
                            &(struct itimerspec){
                                .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 1},
                                .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                            },
                            NULL);
        }
    }

    return timer;
}

void taskio_runtime_add_timer_from(struct taskio_runtime* runtime, struct taskio_timer* timer, bool is_rescheduling) {
    uint64_t time = runtime->hierarchy_wheel.wheels[0].tick * runtime->hierarchy_wheel.wheels[0].resolution;
    if (time >= timer->expiry_time) {
        assert(!is_rescheduling && "rescheduling should not happen when timer has expired");

        timer->handler(timer->data);
        runtime->allocator.free(runtime->allocator.data, timer);
        return;
    }

    uint64_t delay = timer->expiry_time - time;
    struct taskio_wheel_timer* wheel_timer = _find_wheel(runtime, delay);

    if (!is_rescheduling) {
#ifdef TASKIO_RT_MULTI_THREADED_FEATURE
        size_t timer_len = atomic_fetch_add(&runtime->hierarchy_wheel.timer_len, 1);
#else
        size_t timer_len = runtime->hierarchy_wheel.timer_len++;
#endif // TASKIO_RT_MULTI_THREADED_FEATURE

        if (timer_len == 0) {
            timerfd_settime(runtime->timer_fd, 0,
                            &(struct itimerspec){
                                .it_value = (struct timespec){.tv_sec = 0, .tv_nsec = 1},
                                .it_interval = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000},
                            },
                            NULL);
        }
    }

    taskio_wheel_timer_add_from(wheel_timer, timer);
}

static inline struct taskio_wheel_timer* _find_wheel(struct taskio_runtime* runtime, uint64_t delay) {
    size_t ten_ms_index = delay / 10;
    size_t hundred_ms_index = delay / 100;
    size_t second_index = delay / 1000;
    size_t minute_index = delay / (1000 * 60);
    size_t hour_index = delay / (1000 * 60 * 60);
    size_t day_index = delay / (1000 * 60 * 60 * 24);
    size_t year_index = delay / (1000 * 60 * 60 * 24 * 365UL);

    if (year_index > 0) {
        return &runtime->hierarchy_wheel.wheels[7];
    } else if (day_index > 0) {
        return &runtime->hierarchy_wheel.wheels[6];
    } else if (hour_index > 0) {
        return &runtime->hierarchy_wheel.wheels[5];
    } else if (minute_index > 0) {
        return &runtime->hierarchy_wheel.wheels[4];
    } else if (second_index > 0) {
        return &runtime->hierarchy_wheel.wheels[3];
    } else if (hundred_ms_index > 0) {
        return &runtime->hierarchy_wheel.wheels[2];
    } else if (ten_ms_index > 0) {
        return &runtime->hierarchy_wheel.wheels[1];
    } else {
        return &runtime->hierarchy_wheel.wheels[0];
    }
}
