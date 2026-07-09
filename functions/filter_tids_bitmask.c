
#include <limits.h>

#include "../include/delilah.h"

int prog(void* ctx, int mem_size, void* cache, int cache_size) {
    filter_op* op = (filter_op*)ctx;

    uint32_t out_of_place_offset = 0;
    if (!op->inplace) {
        out_of_place_offset = op->file.size / sizeof(uint32_t);
    }

    uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
    uint32_t* data_ptr = (op->use_cache ? (uint32_t*)((char*)cache + op->cached_data_offset) : (data_default_ptr + out_of_place_offset));
    bitmask_t* const result_start_ptr = (bitmask_t* const)data_default_ptr;
    bitmask_t* result_ptr = (bitmask_t*)result_start_ptr;

    op->result_offset = ((char*)result_ptr) - ((char*)ctx);

    if (!op->use_cache && !op->reuse_data) {
        delilah_file_read((char*)data_ptr, op->file.size, op->file.filename);
    }

    uint32_t element_count = op->file.size / sizeof(uint32_t);
    uint32_t const* const data_ptr_end_blocked = data_ptr + (element_count - (element_count & ((sizeof(bitmask_t) * CHAR_BIT) - 1)));
    uint32_t const* const data_ptr_end = data_ptr + element_count;

    uint32_t tuple_idx = 0;

    if (op->comp_type == EQ) {
        uint32_t const comp0 = op->comp0;
        bitmask_t bitmap = 0;
        for (; data_ptr < data_ptr_end_blocked;) {
            bitmap = 0;
            for (bitmask_t i = 0; i < (sizeof(bitmask_t) * CHAR_BIT); ++i) {
                const bitmask_t inc = (*data_ptr == comp0);
                bitmap |= inc << i;
                ++data_ptr;
            }
            *result_ptr++ = bitmap;
        }
        if (data_ptr != data_ptr_end) {
            bitmask_t i = 0;
            bitmap = 0;
            for (; data_ptr != data_ptr_end; ++data_ptr) {
                const bitmask_t inc = (*data_ptr == comp0);
                bitmap |= inc << i++;
            }
            *result_ptr++ = bitmap;
        }
    } else if (op->comp_type == BWI) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        bitmask_t bitmap = 0;
        for (; data_ptr < data_ptr_end_blocked;) {
            bitmap = 0;
            for (bitmask_t i = 0; i < (sizeof(bitmask_t) * CHAR_BIT); ++i) {
                const bitmask_t inc = ((*data_ptr >= comp0) && (*data_ptr <= comp1));
                bitmap |= inc << i;
                ++data_ptr;
            }
            *result_ptr++ = bitmap;
        }
        if (data_ptr != data_ptr_end) {
            bitmask_t i = 0;
            bitmap = 0;
            for (; data_ptr != data_ptr_end; ++data_ptr) {
                const bitmask_t inc = ((*data_ptr >= comp0) && (*data_ptr <= comp1));
                bitmap |= inc << i++;
            }
            *result_ptr++ = bitmap;
        }
    }

    op->result_count = result_ptr - result_start_ptr;
    return 0x0;
}
