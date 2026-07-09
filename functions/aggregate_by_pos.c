
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    aggregation_by_pos_op *op = (aggregation_by_pos_op *)ctx;

    uint32_t *result_ptr = (uint32_t *)((char *)ctx + op->padding_offset);
    uint32_t *positions_ptr = result_ptr + 1;
    
    uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
    uint32_t* data_ptr = (op->use_cache ? (uint32_t*)((char*)cache + op->cached_data_offset) : (positions_ptr + op->available_positions_count));

    op->result_offset = ((char*)result_ptr) - ((char*)ctx);
    
    // if ( !op->use_cache ) {
    //     delilah_file_read((char*)data_ptr, op->file.size, op->file.filename);
    // }

    // Maybe add padding to allow data reuse with larger operator structs
    uint32_t const *const positions_ptr_end = positions_ptr + op->available_positions_count;

    *result_ptr = 0;
    for (; positions_ptr != positions_ptr_end; ++positions_ptr) {
        *result_ptr += data_ptr[*positions_ptr];
    }
    op->result_offset = (char*)result_ptr - (char*)ctx;

    return 0x0;
}
