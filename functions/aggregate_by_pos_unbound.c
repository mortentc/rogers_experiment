
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void *s, int state_size) {
    aggregation_by_pos_op *op = (aggregation_by_pos_op *)ctx;

    uint32_t *result_ptr = (uint32_t *)((char *)ctx + op->padding_offset);
    uint32_t *positions_ptr = result_ptr + 1;

    uint32_t *data_ptr = (uint32_t *)((char *)s + op->cached_data_offset);

    op->result_offset = ((char *)result_ptr) - ((char *)ctx);

    uint32_t current_offset = 0;
    uint64_t read = delilah_file_read_offset(data_ptr, op->file.size, current_offset, op->file.filename);
    current_offset += read;

    // Maybe add padding to allow data reuse with larger operator structs
    uint32_t const *const positions_ptr_end = positions_ptr + op->available_positions_count;

    uint32_t chunk_id = 1;
    const uint32_t normalize_indexes_per_chunk = (read / sizeof(uint32_t));
    uint32_t current_max_index_in_chunk = normalize_indexes_per_chunk;

    *result_ptr = 0;
    for (; positions_ptr != positions_ptr_end; ++positions_ptr) {
        while (*positions_ptr >= current_max_index_in_chunk) {
            read = delilah_file_read_offset(data_ptr, op->file.size, current_offset, op->file.filename);
            current_offset += read;
            if ( read == op->file.size ) {
                current_max_index_in_chunk = normalize_indexes_per_chunk * ++chunk_id;
            } else {
                current_max_index_in_chunk = (normalize_indexes_per_chunk * chunk_id++) + (read / sizeof(uint32_t));
            }
        }
        *result_ptr += data_ptr[*positions_ptr - (normalize_indexes_per_chunk * (chunk_id - 1))];
    }
    op->result_offset = (char *)result_ptr - (char *)ctx;

    return 0x0;
}
