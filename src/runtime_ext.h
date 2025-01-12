#ifndef TASKIO_RUNTIME_EXT_GUARD_HEADER
#define TASKIO_RUNTIME_EXT_GUARD_HEADER

#include <stdint.h>

#include <taskio/runtime.h>

#include "wheel.h"

struct taskio_timer_handle taskio_runtime_add_timer(struct taskio_runtime* runtime, uint64_t delay,
                                                    taskio_wheel_handler handler, void* data);

void taskio_runtime_add_timer_from(struct taskio_runtime* runtime, struct taskio_timer* timer, bool is_rescheduling);

void taskio_runtime_timer_fire(struct taskio_runtime* runtime, struct taskio_timer_handle* handle);
void taskio_runtime_timer_abort(struct taskio_runtime* runtime, struct taskio_timer_handle* handle);

#endif // TASKIO_RUNTIME_EXT_GUARD_HEADER
