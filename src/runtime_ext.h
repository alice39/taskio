#ifndef TASKIO_RUNTIME_EXT_GUARD_HEADER
#define TASKIO_RUNTIME_EXT_GUARD_HEADER

#include "wheel.h"
#include <taskio/runtime.h>

void taskio_runtime_add_timer(struct taskio_runtime* runtime, uint64_t delay, taskio_wheel_handler handler, void* data);
void taskio_runtime_add_timer_from(struct taskio_runtime* runtime, struct taskio_timer* timer);

#endif // TASKIO_RUNTIME_EXT_GUARD_HEADER
