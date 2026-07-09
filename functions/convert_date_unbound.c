
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void *s, int state_size) {
    conversion_op *op = (conversion_op *)ctx;
    struct delilah_io_state_t *state = (struct delilah_io_state_t *)s;

    char *data_default_ptr = ((char *)ctx + op->padding_offset);
    char *data_ptr = (char *)s + op->cached_data_offset;
    uint32_t *result_start_ptr = (uint32_t *)data_default_ptr;
    uint32_t *result_ptr = result_start_ptr;

    op->result_offset = ((char *)result_ptr) - ((char *)ctx);

    uint64_t read = delilah_file_read_offset(data_ptr, op->file.size, state->read, op->file.filename);
    state->read += read;

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
            *result_ptr = val;
            ++result_ptr;
        }
    } else if (op->conv_type == TO_MONTH) {
        for (; data_ptr != data_ptr_end; data_ptr += TUPLE_WIDTH) {
            val ^= val;
            val += (data_ptr[4] - 0x30) * 10;
            val += (data_ptr[5] - 0x30);
            *result_ptr = val;
            ++result_ptr;
        }
    } else if (op->conv_type == TO_DAY) {
        for (; data_ptr != data_ptr_end; data_ptr += TUPLE_WIDTH) {
            val ^= val;
            val += (data_ptr[6] - 0x30) * 10;
            val += (data_ptr[7] - 0x30);
            *result_ptr = val;
            ++result_ptr;
        }
    }
    op->result_count = result_ptr - result_start_ptr;

    return read;
}
