#include <taskio/alloc.h>

static struct taskio_allocator* global_allocator;

void taskio_initialize_allocator(struct taskio_allocator* allocator) { global_allocator = allocator; }

struct taskio_allocator* taskio_default_allocator() { return global_allocator; }
