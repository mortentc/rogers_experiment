
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void *s, int state_size) {
    filter_op * op = (filter_op *)ctx;
    struct delilah_io_state_t *state = (struct delilah_io_state_t *)s;
    
    uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
    uint32_t* data_ptr = (uint32_t*)((char*)s + op->cached_data_offset);
    uint32_t* result_start_ptr = data_default_ptr;
    uint32_t* result_ptr = result_start_ptr;
    *result_ptr = 0;

    op->result_offset = ((char*)result_ptr) - ((char*)ctx);
    
    uint64_t read = delilah_file_read_offset(data_ptr, op->file.size, state->read, op->file.filename);
    state->read += read;

    uint32_t element_count = op->file.size / sizeof(uint32_t);
    uint32_t const * const data_ptr_end = data_ptr + element_count;

    uint32_t tuple_idx = 0;

    // Assuming that branching is expensive, we only branch in the beginning and manually roll out everything else.
    if (op->comp_type == EQ) {
        uint32_t const comp0 = op->comp0;
        for (; data_ptr != data_ptr_end; ++data_ptr, ++tuple_idx) {
            const uint32_t inc = (*data_ptr == comp0) ? 1 : 0;
            (*result_ptr)++;
        }
    }
        
    op->result_count = 1;
    // We cannot return the result count and check on the host side, 
    // because there might be chunks without a match, which returns 0.
    // This could cause the host to abort prematurely.
    return read;
}