#include <taskio/runtime.h>

#include "../../wheel.h"

struct taskio_platform_runtime {
    int event_fd;
    int timer_fd;
    int epoll_fd;

    struct taskio_wheel_timer wheels[8];
};
