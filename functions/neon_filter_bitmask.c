
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
  filter_op * op = (filter_op *)ctx;
  
  uint32_t element_count = op->file.size / sizeof(uint32_t);

  uint32_t out_of_place_offset = 0;
  if (!op->inplace) {
    out_of_place_offset = element_count;
  }

  uint32_t* data_default_ptr = (uint32_t*)((char*)ctx + op->padding_offset);
  uint32_t* data_ptr = (op->use_cache ? (uint32_t*)((char*)cache + op->cached_data_offset) : (data_default_ptr + out_of_place_offset));
  bitmask_t* const result_start_ptr = (bitmask_t* const)data_default_ptr;
  bitmask_t* result_ptr = (bitmask_t*)result_start_ptr;

  op->result_offset = ((char*)result_ptr) - ((char*)ctx);

  if (!op->use_cache && !op->reuse_data) {
    delilah_file_read((char*)data_ptr, op->file.size, op->file.filename);
  }
  
  if (op->comp_type == EQ) {
    op->result_count = delilah_tsl_filter_equal_neon(result_ptr, data_ptr, element_count, op->comp0);
  } else if (op->comp_type == BWI) {
    op->result_count = delilah_tsl_filter_between_neon(result_ptr, data_ptr, element_count, op->comp0, op->comp1);
  }

  return 0x0;
}