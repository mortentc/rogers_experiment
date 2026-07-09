
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    memory_bench_op* op = (memory_bench_op*)ctx;

    const uint32_t offset = (op->use_cache ? op->data_offset : op->padding_offset);
    uint32_t * const data_ptr  = (uint32_t*)(op->use_cache ? ((char*) cache + offset) : ((char*)ctx + offset));

    const uint32_t elem_cnt = op->bytes / sizeof(uint32_t);
    for ( uint32_t idx = 0; idx < elem_cnt; ++idx ) {
        data_ptr[idx] = idx;
    }

    return 0x0;
}
