#include <assert.h>
#include "memarena.h"

void ma_init_allocator_linear(void *addr, size_t size, ma_ctx *ctx) {
    ctx->type = MA_LINEAR;
    ctx->used = 0;
    ctx->memory = addr;
    ctx->size = size;
    ctx->alloc_data = NULL;
}

ma_ctx ma_create_allocator_linear(void *addr, size_t size) {
    ma_ctx result;
    ma_init_allocator_linear(addr, size, &result);
    return result;
}

void ma_init_allocator_stack(void *addr, size_t size, ma_ctx *ctx) {
    ctx->type = MA_STACK;
    ctx->used = 0;
    ctx->memory = addr;
    ctx->size = size;
    ctx->alloc_data = NULL;
}

ma_ctx ma_create_allocator_stack(void *addr, size_t size) {
    ma_ctx result;
    ma_init_allocator_stack(addr, size, &result);
    return result;
}

void ma_init_allocator_freelist(void *addr, size_t size, ma_ctx *ctx) {
    ma_alloc_freelist *freelist_alloc_data = (ma_alloc_freelist *)addr;
    freelist_alloc_data->freelist = NULL;

    ctx->type = MA_FREELIST;
    ctx->used = sizeof(ma_alloc_freelist);
    ctx->memory = (unsigned char *)addr + sizeof(ma_alloc_freelist);
    ctx->size = size;
    ctx->alloc_data = freelist_alloc_data;
}

ma_ctx ma_create_allocator_freelist(void *addr, size_t size) {
    ma_ctx result;
    ma_init_allocator_freelist(addr, size, &result);
    return result;
}

void ma_init_allocator_pool(void *addr, size_t total_size, size_t chunk_size, ma_ctx *ctx) {
    assert(total_size > chunk_size);

    ma_alloc_pool *pool_alloc_data = (ma_alloc_pool *)addr;
    pool_alloc_data->chunk_size = chunk_size;
    pool_alloc_data->used = NULL;

    size_t num_pool_elements = (total_size - sizeof(ma_alloc_pool)) / (chunk_size + sizeof(ma_alloc_pool_entry));
    assert(num_pool_elements > 0);

    ctx->type = MA_POOL;
    ctx->used = sizeof(ma_alloc_pool);
    ctx->size = total_size;
    ctx->alloc_data = pool_alloc_data;

    unsigned char *memory = (unsigned char *)addr + sizeof(ma_alloc_pool);
    ma_alloc_pool_entry *prev = NULL;

    for (size_t i = 0; i < num_pool_elements; i++) {
        ma_alloc_pool_entry *entry = (ma_alloc_pool_entry *)memory;
        entry->next = NULL;
        if (prev)
            prev->next = entry;
        else
            pool_alloc_data->free = entry;
        prev = entry;
        memory += chunk_size + sizeof(ma_alloc_pool_entry);
    }
}

ma_ctx ma_create_allocator_pool(void *addr, size_t total_size, size_t chunk_size) {
    ma_ctx result;
    ma_init_allocator_pool(addr, total_size, chunk_size, &result);
    return result;
}

void *ma_alloc(ma_ctx *ctx, size_t size) {

    assert (size > 0);

    void *result;

    ma_alloc_freelist *freelist_alloc_data;
    ma_alloc_freelist_entry *freelist_entry, *freelist_entry_prev, *best_freelist_entry, *best_freelist_entry_prev;

    ma_alloc_pool *pool_alloc_data;
    ma_alloc_pool_entry *pool_entry;

    switch (ctx->type) {
    case MA_LINEAR:
    case MA_STACK:
        assert(ctx->used + size <= ctx->size);
        result = (unsigned char *)ctx->memory + ctx->used;
        ctx->used += size;
        break;
    case MA_FREELIST:

        freelist_alloc_data = (ma_alloc_freelist *)ctx->alloc_data;
        freelist_entry = freelist_alloc_data->freelist;
        freelist_entry_prev = NULL;
        best_freelist_entry = NULL;
        best_freelist_entry_prev = NULL;

        while (freelist_entry) {
            if (freelist_entry->chunk_size == size) {
                // can't get a better fit
                best_freelist_entry = freelist_entry;
                best_freelist_entry_prev = freelist_entry_prev;
                break;
            } else if (freelist_entry->chunk_size > size) {
                // candidate, check if better fit
                if (!best_freelist_entry || freelist_entry->chunk_size < best_freelist_entry->chunk_size) {
                    best_freelist_entry = freelist_entry;
                    best_freelist_entry_prev = freelist_entry_prev;
                }
            }
            freelist_entry_prev = freelist_entry;
            freelist_entry = freelist_entry->next;
        }

        if (best_freelist_entry) {
            if (best_freelist_entry_prev) {
                best_freelist_entry_prev->next = best_freelist_entry->next;
            } else {
                freelist_alloc_data->freelist = best_freelist_entry->next;
            }
            result = (unsigned char *)best_freelist_entry + sizeof(ma_alloc_freelist_entry);
            best_freelist_entry->next = NULL;
        } else {
            assert(ctx->used + size + sizeof(ma_alloc_freelist_entry) <= ctx->size);

            freelist_entry = (ma_alloc_freelist_entry *)((unsigned char *)ctx->memory + ctx->used);
            freelist_entry->chunk_size = size;
            freelist_entry->next = NULL;
            result = (unsigned char *)ctx->memory + ctx->used + sizeof(ma_alloc_freelist_entry);

            ctx->used += size + sizeof(ma_alloc_freelist_entry);
        }

        break;

    case MA_POOL:

        pool_alloc_data = (ma_alloc_pool *)ctx->alloc_data;

        assert(ctx->used + pool_alloc_data->chunk_size <= ctx->size);
        assert(pool_alloc_data->free);
        assert(size <= pool_alloc_data->chunk_size);

        pool_entry = pool_alloc_data->free;
        pool_alloc_data->free = pool_entry->next;
        pool_entry->next = pool_alloc_data->used;
        pool_alloc_data->used = pool_entry;

        ctx->used += pool_alloc_data->chunk_size + sizeof(ma_alloc_pool_entry);

        result = (unsigned char *)pool_entry + sizeof(ma_alloc_pool_entry);

        break;

    default:
        assert(!"Unknown type");

    }

    return result;
}

void ma_free(ma_ctx *ctx, void *addr) {
    ma_alloc_freelist_entry *freelist_entry;
    ma_alloc_freelist *freelist_alloc_data;

    ma_alloc_pool_entry *pool, *prev;
    ma_alloc_pool *pool_alloc_data;

    switch (ctx->type) {
    case MA_LINEAR:
        break;
    case MA_STACK:
        ctx->used -= ((unsigned char *)ctx->memory + ctx->used) - (unsigned char *)addr;
        break;
    case MA_FREELIST:

        freelist_alloc_data = (ma_alloc_freelist *)ctx->alloc_data;

        freelist_entry = (ma_alloc_freelist_entry *)((unsigned char *)addr - sizeof(ma_alloc_freelist_entry));
        freelist_entry->next = freelist_alloc_data->freelist;
        freelist_alloc_data->freelist = freelist_entry;

        break;

    case MA_POOL:

        pool_alloc_data = (ma_alloc_pool *)ctx->alloc_data;
        prev = NULL;
        pool = pool_alloc_data->used;

        while (1) {
            assert(pool);

            if (((unsigned char *)pool + sizeof(ma_alloc_pool_entry)) == addr) {
                if (prev) {
                    prev->next = pool->next;
                } else {
                    pool_alloc_data->used = pool->next;
                }
                pool->next = pool_alloc_data->free;
                pool_alloc_data->free = pool;

                ctx->used -= (pool_alloc_data->chunk_size + sizeof(ma_alloc_pool_entry));

                break;
            }

            prev = pool;
            pool = pool->next;
        }

        break;
    }
}

ma_linear_snapshot *ma_snapshot_save(ma_ctx *ctx) {
    assert(ctx->type == MA_LINEAR);
    ma_linear_snapshot *result = ma_alloc_struct(ctx, ma_linear_snapshot);
    result->ctx = ctx;
    result->prev_used = ctx->used;
    return result;
}

void ma_snapshot_restore(ma_linear_snapshot *snapshot) {
    snapshot->ctx->used = snapshot->prev_used;
}
