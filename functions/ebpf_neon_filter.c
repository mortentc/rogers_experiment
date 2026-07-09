
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    filter_op * op = (filter_op *)ctx;
    uint32_t len = op->file.size / sizeof(uint32_t);

    uint32_t* data = (uint32_t*)((char*)ctx + sizeof(filter_op));

	uint32_t result = 0;
	for ( uint32_t i = 0; i < len; i++ ) {
		if ((data[i] >= op->comp0 ) && (data[i] <= op->comp1)) {
			++result;
		}
	}

    op->result_count = result;

    return 0x0;
}