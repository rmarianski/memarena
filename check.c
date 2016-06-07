#include <string.h>
#include <assert.h>
#include "memarena.h"

void check_push() {
    ma_arena *arena = ma_init(64);
    char *data = ma_push(arena, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);
    ma_destroy(arena);
}

void check_snapshot() {
    ma_arena *arena = ma_init(128);
    char *data = ma_push(arena, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);

    size_t used = arena->used;
    ma_snapshot *snapshot = ma_snapshot_save(arena);
    char *chunk = ma_push(arena, 4);
    assert(strcmp(chunk, data) != 0);
    ma_snapshot_restore(snapshot);
    char *data_loc = (char *)arena->base + used - 4;
    assert(data == data_loc);

    ma_destroy(arena);
}

int main() {
    check_push();
    check_snapshot();
}
