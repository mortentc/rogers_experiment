
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    filter_op * op = (filter_op *)ctx;
    uint32_t len = op->file.size / sizeof(uint32_t);

    op->result_count = delilah_tsl_filter_sequential(ctx + sizeof(filter_op), len, ctx + sizeof(filter_op) + op->file.size, op->comp0, op->comp1);

    return 0x0;
}