
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    cache_file_op *op = (cache_file_op *)ctx;

    // Maybe add padding to allow data reuse with larger operator structs
    char *data_ptr = ((char *)cache + op->data_offset);
    op->bytes_cached = delilah_file_read((char *)data_ptr, op->file.size, op->file.filename);
    
    return 0x0;
}
