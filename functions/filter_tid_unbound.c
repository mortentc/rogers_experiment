
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void *s, int state_size) {
    filter_op * op = (filter_op *)ctx;
    
    // struct delilah_io_state_t *state = (struct delilah_io_state_t *)s;
    // To allow for concurrent executions we need multiple states.
    struct delilah_io_state_t *state = (struct delilah_io_state_t *)((char*)s + op->cache_offset);
    
    uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
    uint32_t* data_ptr = (uint32_t*)((char*)s + op->cached_data_offset);
    uint32_t* result_start_ptr = data_default_ptr;
    uint32_t* result_ptr = result_start_ptr;

    op->result_offset = ((char*)result_ptr) - ((char*)ctx);
        
    uint64_t read = delilah_file_read_offset(data_ptr, op->file.size, state->read, op->file.filename);
    
    const uint32_t current_chunk_idx = state->read / op->file.size;
    const uint32_t normalize_indexes_per_chunk = (op->file.size / sizeof(uint32_t));
    
    state->read += read;

    uint32_t element_count = read / sizeof(uint32_t);
    uint32_t const * const data_ptr_end = data_ptr + element_count;

    uint32_t tuple_idx = current_chunk_idx * normalize_indexes_per_chunk;

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
    // We cannot return the result count and check on the host side, 
    // because there might be chunks without a match, which returns 0.
    // This could cause the host to abort prematurely.
    return read;
}
