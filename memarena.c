#include <stdlib.h>
#include <assert.h>
#include "memarena.h"

ma_arena *ma_init(size_t size) {
    assert(size > sizeof(ma_arena));
    ma_arena *result = calloc(1, size);
    void *base = (char *)result + sizeof(ma_arena);
    result->base = base;
    result->used = sizeof(ma_arena);
    result->size = size;
    return result;
}

void ma_destroy(ma_arena *arena) {
    arena->used = 0;
    arena->size = 0;
    free((char *)arena->base - sizeof(ma_arena));
}

void *ma_push(ma_arena *arena, size_t size) {
    assert(arena->used + sizeof(ma_arena) + size < arena->size);
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
