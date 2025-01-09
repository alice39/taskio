#ifndef TASKIO_COMMON_EXT_GUARD_HEADER
#define TASKIO_COMMON_EXT_GUARD_HEADER

#include <taskio/common.h>

struct taskio_join_task {
    size_t index;
    struct taskio_future* future;

    struct taskio_join_ext_env* env;
    struct taskio_join_task* next;
};

future_fn(size_t, taskio_join_ext)(struct taskio_allocator* allocator, void* out, size_t len,
                                   struct taskio_join_task* head, taskio_join_on_ready on_ready,
                                   taskio_join_on_finish on_finish, taskio_join_on_cleanup on_cleanup);

#endif // TASKIO_COMMON_EXT_GUARD_HEADER
