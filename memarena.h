#ifndef MEM_ARENA_H
#define MEM_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *base;
    size_t used;
    size_t size;
} ma_arena;

typedef struct {
    ma_arena *arena;
    size_t used;
} ma_snapshot;

ma_arena *ma_init(size_t size);
void ma_destroy(ma_arena *arena);

void *ma_push(ma_arena *arena, size_t size);
#define ma_push_struct(arena, structure) ma_push(arena, sizeof(structure))

ma_snapshot *ma_snapshot_save(ma_arena *arena);
void ma_snapshot_restore(ma_snapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif
