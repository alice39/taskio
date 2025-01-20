#ifndef TASKIO_COLLECTION_VEC_GUARD_HEADER
#define TASKIO_COLLECTION_VEC_GUARD_HEADER

#include <stddef.h>
#include <string.h>

enum taskio_predicate {
    taskio_predicate_true,
    taskio_predicate_false,
    taskio_predicate_true_finish,
    taskio_predicate_false_finish,
};

#define taskio_vec(T) struct taskio_vec_##T

#define taskio_vec_impl(T)                                                                                             \
    struct taskio_vec_##T {                                                                                            \
        size_t len;                                                                                                    \
        size_t capacity;                                                                                               \
        T* data;                                                                                                       \
    }

#define taskio_vec_implS(T)                                                                                            \
    struct taskio_vec_##T {                                                                                            \
        size_t len;                                                                                                    \
        size_t capacity;                                                                                               \
        struct T* data;                                                                                                \
    }

#define taskio_vec_init(vec)                                                                                           \
    (vec)->len = 0;                                                                                                    \
    (vec)->capacity = 0;                                                                                               \
    (vec)->data = NULL;

#define taskio_vec_move(dst_vec, src_vec)                                                                              \
    (dst_vec)->len = (src_vec)->len;                                                                                   \
    (dst_vec)->capacity = (src_vec)->capacity;                                                                         \
    (dst_vec)->data = (src_vec)->data;                                                                                 \
                                                                                                                       \
    (src_vec)->len = 0;                                                                                                \
    (src_vec)->capacity = 0;                                                                                           \
    (src_vec)->data = NULL;

#define taskio_vec_destroy(vec, allocator, ...) taskio_vec_clear(vec, allocator, __VA_ARGS__)

#define taskio_vec_empty(vec) (vec)->len == 0
#define taskio_vec_len(vec) (vec)->len

#define taskio_vec_shrink(vec, allocator)                                                                              \
    if ((vec)->capacity > (vec)->len) {                                                                                \
        (vec)->capacity = (vec)->len;                                                                                  \
                                                                                                                       \
        if ((vec)->len == 0) {                                                                                         \
            (vec)->data = NULL;                                                                                        \
        } else {                                                                                                       \
            typeof((vec)->data) new_data =                                                                             \
                (allocator)->alloc((allocator)->data, sizeof(typeof(*(vec)->data)) * (vec)->len);                      \
            memcpy(new_data, (vec)->data, sizeof(typeof(*(vec)->data)) * (vec)->len);                                  \
                                                                                                                       \
            (allocator)->free((allocator)->data, new_data);                                                            \
            (vec)->data = new_data;                                                                                    \
        }                                                                                                              \
    }

#define taskio_vec_clear(vec, allocator, ...)                                                                          \
    __VA_OPT__(for (size_t _vec_i_var = 0; _vec_i_var < (vec)->len;                                                    \
                    _vec_i_var++) { __VA_ARGS__(taskio_vec_at(vec, _vec_i_var)); })                                    \
    (allocator)->free((allocator)->data, (vec)->data);                                                                 \
                                                                                                                       \
    (vec)->len = 0;                                                                                                    \
    (vec)->capacity = 0;                                                                                               \
    (vec)->data = NULL;

#define taskio_vec_at(vec, index) (vec)->data[index]

#define taskio_vec_push_last(vec, allocator, obj)                                                                      \
    if ((vec)->len >= (vec)->capacity) {                                                                               \
        size_t new_capacity = (vec)->capacity > 0 ? (vec)->capacity * 2 : 16;                                          \
        typeof((vec)->data) new_data =                                                                                 \
            (allocator)->alloc((allocator)->data, sizeof(typeof(*(vec)->data)) * new_capacity);                        \
                                                                                                                       \
        if ((vec)->data) {                                                                                             \
            memcpy(new_data, (vec)->data, sizeof(typeof(*(vec)->data)) * (vec)->capacity);                             \
            (allocator)->free((allocator)->data, (vec)->data);                                                         \
        }                                                                                                              \
                                                                                                                       \
        (vec)->capacity = new_capacity;                                                                                \
        (vec)->data = new_data;                                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    taskio_vec_at(vec, (vec)->len++) = obj;

#define taskio_vec_pop_last(vec, allocator)                                                                            \
    taskio_vec_at(vec, --(vec)->len);                                                                                  \
                                                                                                                       \
    if ((vec)->capacity / 2 >= (vec)->len) {                                                                           \
        size_t new_capacity = ((vec)->capacity * 3) / 4;                                                               \
                                                                                                                       \
        if (new_capacity == 0) {                                                                                       \
            taskio_vec_clear(T, vec, allocator);                                                                       \
        } else {                                                                                                       \
            typeof((vec)->data) new_data =                                                                             \
                (allocator)->alloc((allocator)->data, sizeof(typeof(*(vec)->data)) * new_capacity);                    \
                                                                                                                       \
            if ((vec)->data) {                                                                                         \
                memcpy(new_data, (vec)->data, sizeof(typeof(*(vec)->data)) * (vec)->capacity);                         \
                (allocator)->free((allocator)->data, (vec)->data);                                                     \
            }                                                                                                          \
                                                                                                                       \
            (vec)->capacity = new_capacity;                                                                            \
            (vec)->data = new_data;                                                                                    \
        }                                                                                                              \
    }

#define taskio_vec_remove_if(vec, allocator, predicate, predicate_data)                                                \
    {                                                                                                                  \
        bool _vec_var_running = true;                                                                                  \
                                                                                                                       \
        for (size_t _vec_var_i = 0; _vec_var_i < (vec)->len && _vec_var_running; _vec_var_i++) {                       \
            switch (predicate(&taskio_vec_at(vec, _vec_var_i), predicate_data)) {                                      \
                case taskio_predicate_true_finish: {                                                                   \
                    _vec_var_running = false;                                                                          \
                    [[fallthrough]];                                                                                   \
                }                                                                                                      \
                case taskio_predicate_true: {                                                                          \
                    size_t _vec_var_move_len = (vec)->len - _vec_var_i - 1;                                            \
                    if (_vec_var_move_len > 0) {                                                                       \
                        memmove((vec)->data + _vec_var_i, (vec)->data + _vec_var_i + 1,                                \
                                sizeof(typeof(*(vec)->data)) * _vec_var_move_len);                                     \
                    }                                                                                                  \
                    (vec)->len -= 1;                                                                                   \
                    _vec_var_i -= 1;                                                                                   \
                    break;                                                                                             \
                }                                                                                                      \
                case taskio_predicate_false_finish: {                                                                  \
                    _vec_var_running = false;                                                                          \
                    [[fallthrough]];                                                                                   \
                }                                                                                                      \
                case taskio_predicate_false: {                                                                         \
                    break;                                                                                             \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    }

#endif // TASKIO_COLLECTION_VEC_GUARD_HEADER
