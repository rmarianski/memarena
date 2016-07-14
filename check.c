#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "memarena.h"

void check_alloc_linear(void *mem) {
    ma_ctx ctx = ma_create_allocator_linear(mem, 64);
    char *data = ma_alloc(&ctx, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);
}

void check_snapshot(void *mem) {
    ma_ctx ctx = ma_create_allocator_linear(mem, 128);
    char *data = ma_alloc(&ctx, 4);
    strncpy(data, "123", 4);
    assert(strcmp(data, "123") == 0);

    size_t used = ctx.used;
    ma_linear_snapshot *snapshot = ma_snapshot_save(&ctx);
    char *chunk = ma_alloc(&ctx, 4);
    assert(strcmp(chunk, data) != 0);
    ma_snapshot_restore(snapshot);
    char *data_loc = (char *)ctx.memory + used - 4;
    assert(data == data_loc);
}

void check_alloc_stack(void *mem) {
    ma_ctx ctx = ma_create_allocator_stack(mem, 128);

    char *data1 = ma_alloc(&ctx, 16);
    ma_free(&ctx, data1);
    char *data2 = ma_alloc(&ctx, 16);
    assert(data1 == data2);
}

void check_alloc_stack_repeated(void *mem) {
    ma_ctx ctx = ma_create_allocator_stack(mem, 256);

    void *addr[10];

    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
    }
    for (int i = 0; i < 10; i++) {
        ma_free(&ctx, addr[10 - 1 - i]);
    }

    assert(ctx.used == 0);
    assert(ctx.memory == mem);

    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
        ma_free(&ctx, addr[i]);
    }

    assert(ctx.used == 0);
    assert(ctx.memory == mem);

}

void check_alloc_freelist(void *mem) {
    ma_ctx ctx = ma_create_allocator_freelist(mem, 256);
    void *freelistmem = ma_alloc(&ctx, 10);
    ma_free(&ctx, freelistmem);
    assert(ctx.used == 10 + sizeof(ma_alloc_freelist) + sizeof(ma_alloc_freelist_entry));
}

void check_alloc_freelist_repeated(void *mem) {
    ma_ctx ctx = ma_create_allocator_freelist(mem, 1024);
    void *addr[10];
    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
        ma_free(&ctx, addr[i]);
    }
    assert(ctx.used == 10 + sizeof(ma_alloc_freelist) + sizeof(ma_alloc_freelist_entry));

    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
    }
    for (int i = 0; i < 10; i++) {
        ma_free(&ctx, addr[i]);
    }
    assert(ctx.used == 10 * 10 + sizeof(ma_alloc_freelist) + 10 * sizeof(ma_alloc_freelist_entry));
}

void check_alloc_freelist_bestfit(void *mem) {
    ma_ctx ctx = ma_create_allocator_freelist(mem, 256);
    void *addr1, *addr2;
    addr1 = ma_alloc(&ctx, 5);
    addr2 = ma_alloc(&ctx, 10);
    ma_free(&ctx, addr1);
    ma_free(&ctx, addr2);

    addr1 = ma_alloc(&ctx, 5);
    addr2 = ma_alloc(&ctx, 10);
    assert(ctx.used == 10 + 5 + sizeof(ma_alloc_freelist) + 2 * sizeof(ma_alloc_freelist_entry));

    ma_free(&ctx, addr1);
    ma_free(&ctx, addr2);

    assert(ctx.used == 10 + 5 + sizeof(ma_alloc_freelist) + 2 * sizeof(ma_alloc_freelist_entry));
}

void check_alloc_pool(void *mem) {
    ma_ctx ctx = ma_create_allocator_pool(mem, 64, 10);
    assert(ctx.used == sizeof(ma_alloc_pool));

    void *addr = ma_alloc(&ctx, 10);
    assert(ctx.used == sizeof(ma_alloc_pool) + sizeof(ma_alloc_pool_entry) + 10);
    ma_free(&ctx, addr);

    assert(ctx.used == sizeof(ma_alloc_pool));
}

void check_alloc_pool_repeated(void *mem) {
    ma_ctx ctx = ma_create_allocator_pool(mem, 2048, 10);
    assert(ctx.used == sizeof(ma_alloc_pool));

    void *addr[10];

    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
        assert(ctx.used == sizeof(ma_alloc_pool) + sizeof(ma_alloc_pool_entry) + 10);
        ma_free(&ctx, addr[i]);
    }
    assert(ctx.used == sizeof(ma_alloc_pool));

    for (int i = 0; i < 10; i++) {
        addr[i] = ma_alloc(&ctx, 10);
        assert(ctx.used == sizeof(ma_alloc_pool) + (sizeof(ma_alloc_pool_entry) + 10) * (i + 1));
    }

    for (int i = 0; i < 10; i++) {
        ma_free(&ctx, addr[i]);
        assert(ctx.used == sizeof(ma_alloc_pool) + (10 - 1 - i ) * (sizeof(ma_alloc_pool_entry) + 10));
    }

    assert(ctx.used == sizeof(ma_alloc_pool));
}

typedef struct {
    unsigned int allocs, frees;
} custom_alloc_data;

void *custom_alloc(ma_ctx *ctx, size_t size) {
    ma_alloc_custom *custom = (ma_alloc_custom *)ctx->alloc_data;
    custom_alloc_data *data = (custom_alloc_data *)custom->custom_alloc_data;
    data->allocs++;
    return NULL;
}

void custom_free(ma_ctx *ctx, void *addr) {
    ma_alloc_custom *custom = (ma_alloc_custom *)ctx->alloc_data;
    custom_alloc_data *data = (custom_alloc_data *)custom->custom_alloc_data;
    data->frees++;
}

void check_alloc_custom(void *mem) {
    custom_alloc_data data = {0};
    ma_alloc_custom custom_data = {
        .alloc = custom_alloc,
        .free = custom_free,
        .custom_alloc_data = &data,
    };
    ma_ctx ctx = {
        .type = MA_CUSTOM,
        .memory = mem,
        .size = 1024,
        .alloc_data = &custom_data,
    };

    assert(data.allocs == 0);
    assert(data.frees == 0);

    ma_alloc(&ctx, 10);
    assert(data.allocs == 1);
    assert(data.frees == 0);

    ma_free(&ctx, NULL);
    assert(data.allocs == 1);
    assert(data.frees == 1);
}

int main() {
    void *mem = malloc(4096);

    check_alloc_linear(mem);
    check_snapshot(mem);

    check_alloc_stack(mem);
    check_alloc_stack_repeated(mem);

    check_alloc_freelist(mem);
    check_alloc_freelist_repeated(mem);
    check_alloc_freelist_bestfit(mem);

    check_alloc_pool(mem);
    check_alloc_pool_repeated(mem);

    check_alloc_custom(mem);

    free(mem);
}
