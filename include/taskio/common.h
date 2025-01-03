#ifndef TASKIO_COMMON_GUARD_HEADER
#define TASKIO_COMMON_GUARD_HEADER

#include <stdint.h>

#include "async.h"

struct taskio_sleep_env {
    uint64_t ms;
    struct taskio_future_context ctx;
};

future_fn(void, taskio_sleep)(uint64_t ms);

#endif // TASKIO_COMMON_GUARD_HEADER
