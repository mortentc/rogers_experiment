
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    read_file_op *op = (read_file_op *)ctx;

    char *data_ptr = ((char *)ctx + op->padding_offset + op->data_offset);
    op->result_count = delilah_file_read((char *)data_ptr, op->file.size, op->file.filename);
    op->result_offset = data_ptr - (char*)ctx;
    
    return 0x0;
}
