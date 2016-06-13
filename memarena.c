#include <assert.h>
#include "memarena.h"

ma_arena ma_create(void *addr, size_t size) {
    ma_arena result;
    ma_init(addr, size, &result);
    return result;
}

void ma_init(void *addr, size_t size, ma_arena *arena) {
    arena->base = addr;
    arena->size = size;
    arena->used = 0;
}

void *ma_push(ma_arena *arena, size_t size) {
    assert(arena->used + size < arena->size);
    void *result = (unsigned char *)arena->base + arena->used;
    arena->used += size;
    return result;
}

ma_snapshot *ma_snapshot_save(ma_arena *arena) {
    size_t used = arena->used;
    ma_snapshot *result = ma_push_struct(arena, ma_snapshot);
    result->arena = arena;
    result->used = used;
    return result;
}

void ma_snapshot_restore(ma_snapshot *snapshot) {
    ma_arena *arena = snapshot->arena;
    arena->used = snapshot->used;
}
