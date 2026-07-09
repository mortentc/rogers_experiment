
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    filter_op * op = (filter_op *)ctx;
    
    uint32_t out_of_place_offset = 0;
    if ( !op->inplace ) {
        out_of_place_offset = op->file.size / sizeof(uint32_t);
    }

    uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
    uint32_t* data_ptr = (op->use_cache ? (uint32_t*)((char*)cache + op->cached_data_offset) : (data_default_ptr + out_of_place_offset));
    uint32_t* result_start_ptr = data_default_ptr;
    uint32_t* result_ptr = result_start_ptr;

    op->result_offset = ((char*)result_ptr) - ((char*)ctx);
    
    if ( !op->use_cache && !op->reuse_data ) {
        delilah_file_read((char*)data_ptr, op->file.size, op->file.filename);
    }

    uint32_t element_count = op->file.size / sizeof(uint32_t);
    uint32_t const * const data_ptr_end = data_ptr + element_count;

    uint32_t tuple_idx = 0;

    // Assuming that branching is expensive, we only branch in the beginning and manually roll out everything else.
    if (op->comp_type == EQ) {
        uint32_t const comp0 = op->comp0;
        for (; data_ptr != data_ptr_end; ++data_ptr, ++tuple_idx) {
            const uint32_t inc = (*data_ptr == comp0) ? 1 : 0;
            *result_ptr = tuple_idx;
            result_ptr += inc;
        }
    } else if (op->comp_type == BWI) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        for (; data_ptr != data_ptr_end; ++data_ptr, ++tuple_idx) {
            const uint32_t inc = ((*data_ptr >= comp0) && (*data_ptr <= comp1)) ? 1 : 0;
            *result_ptr = tuple_idx;
            result_ptr += inc;
        }
    }
        
    op->result_count = result_ptr - result_start_ptr;
    return op->result_count;
}


 
