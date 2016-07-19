#ifndef MEM_ARENA_H
#define MEM_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// allocators:
//   - linear
//   - stack
//   - freelist
//   - memory pool
//   - custom

typedef enum {
    MA_LINEAR,
    MA_STACK,
    MA_FREELIST,
    MA_POOL,
    MA_CUSTOM,
} ma_alloc_type;

typedef struct {
    ma_alloc_type type;
    void *memory;
    size_t size;
    size_t used;
    void *alloc_data;
} ma_ctx;

// linear doesn't need custom allocator space, nor does stack

typedef struct ma_alloc_freelist_entry {
    // the memory is assumed to be located immediately after the entry
    size_t chunk_size;
    struct ma_alloc_freelist_entry *next;
} ma_alloc_freelist_entry;

typedef struct ma_alloc_freelist {
    ma_alloc_freelist_entry *freelist;
} ma_alloc_freelist;

typedef struct ma_alloc_pool_entry {
    // the memory is assumed to be located immediately after the entry
    struct ma_alloc_pool_entry *next;
} ma_alloc_pool_entry;

typedef struct {
    size_t chunk_size;
    ma_alloc_pool_entry *used;
    ma_alloc_pool_entry *free;
} ma_alloc_pool;

typedef struct {
    void *(*alloc)(ma_ctx *ctx, size_t size);
    void (*free)(ma_ctx *ctx, void *addr);
    void *custom_alloc_data;
} ma_alloc_custom;

typedef struct {
    ma_ctx *ctx;
    size_t prev_used;
} ma_linear_snapshot;

void *ma_alloc(ma_ctx *ctx, size_t size);
#define ma_alloc_struct(ctx, structure) ma_alloc(ctx, sizeof(structure))
void ma_free(ma_ctx *ctx, void *addr);

ma_ctx *ma_create_allocator_linear(void *addr, size_t size);
ma_ctx *ma_create_allocator_stack(void *addr, size_t size);
ma_ctx *ma_create_allocator_freelist(void *addr, size_t size);
ma_ctx *ma_create_allocator_pool(void *addr, size_t size, size_t chunk_size);

// responsible for initializing custom data part manually
ma_ctx *ma_create_allocator_custom(void *addr, size_t size);

ma_linear_snapshot *ma_snapshot_save(ma_ctx *ctx);
void ma_snapshot_restore(ma_linear_snapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif
