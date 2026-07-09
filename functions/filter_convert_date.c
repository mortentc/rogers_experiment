
#include "../include/delilah.h"

static inline int is_match(uint32_t val, COMP_TYPE comp_type, uint32_t comp0, uint32_t comp1 ) {
    if (comp_type == EQ) {
        return (val == comp0) ? 1 : 0;
    } else if (comp_type == BWI) {
        return ((val >= comp0) && (val <= comp1)) ? 1 : 0;
    } else {
        // We should throw here.
        return 1337;
    }
}

int prog(void *ctx, int mem_size, void *s, int state_size) {
    fused_filter_conversion_op *op = (fused_filter_conversion_op *)ctx;
    struct delilah_io_state_t *state = (struct delilah_io_state_t *)s;

    char *data_default_ptr = ((char *)ctx + op->padding_offset);
    char *data_ptr = (char *)s + op->cached_data_offset;
    uint32_t *result_start_ptr = (uint32_t *)data_default_ptr;
    uint32_t *result_ptr = result_start_ptr;

    op->result_offset = ((char *)result_ptr) - ((char *)ctx);

    uint64_t read = op->file.size;//delilah_file_read_offset(data_ptr, op->file.size, state->read, op->file.filename);
    state->read += read;

    uint32_t element_count = op->file.size / sizeof(uint32_t);

    uint32_t tuple_idx = 0;
    // We assume a file layout of 8 chars to represent a date, e.g. 19960505, i.e. YYYY-MM-DD
    const uint32_t TUPLE_WIDTH = 8;
    char *data_ptr_end = data_ptr + read;

    // Assuming that branching is expensive, we only branch in the beginning and manually roll out everything else.
    uint32_t val = 0;
    if (op->conv_type == TO_YEAR) {
        for (; data_ptr != data_ptr_end; data_ptr += TUPLE_WIDTH) {
            val ^= val;
            val += (data_ptr[0] - 0x30) * 1000;
            val += (data_ptr[1] - 0x30) * 100;
            val += (data_ptr[2] - 0x30) * 10;
            val += (data_ptr[3] - 0x30);
            *result_ptr = tuple_idx++;
            result_ptr += is_match( val, op->comp_type, op->comp0, op->comp1);
        }
    } else if (op->conv_type == TO_MONTH) {
        for (; data_ptr != data_ptr_end; data_ptr += TUPLE_WIDTH) {
            val ^= val;
            val += (data_ptr[4] - 0x30) * 10;
            val += (data_ptr[5] - 0x30);
            *result_ptr = tuple_idx++;
            result_ptr += is_match( val, op->comp_type, op->comp0, op->comp1);
        }
    } else if (op->conv_type == TO_DAY) {
        for (; data_ptr != data_ptr_end; data_ptr += TUPLE_WIDTH) {
            val ^= val;
            val += (data_ptr[6] - 0x30) * 10;
            val += (data_ptr[7] - 0x30);
            *result_ptr = tuple_idx++;
            result_ptr += is_match( val, op->comp_type, op->comp0, op->comp1);
        }
    }
    op->result_count = result_ptr - result_start_ptr;

    return read;
}
