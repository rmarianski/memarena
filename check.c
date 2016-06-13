#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "memarena.h"

void check_push() {
    void *mem = calloc(1, 64);
    ma_arena arena = ma_create(mem, 64);
    char *data = ma_push(&arena, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);
    free(mem);
}

void check_snapshot() {
    void *mem = calloc(1, 128);
    ma_arena arena = ma_create(mem, 128);
    char *data = ma_push(&arena, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);

    size_t used = arena.used;
    ma_snapshot *snapshot = ma_snapshot_save(&arena);
    char *chunk = ma_push(&arena, 4);
    assert(strcmp(chunk, data) != 0);
    ma_snapshot_restore(snapshot);
    char *data_loc = (char *)arena.base + used - 4;
    assert(data == data_loc);
    free(mem);
}

int main() {
    check_push();
    check_snapshot();
}
