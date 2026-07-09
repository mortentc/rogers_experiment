
#include "../include/delilah.h"

int prog(void *ctx, int mem_size, void* cache, int cache_size) {
    cache_file_op *op = (cache_file_op *)ctx;

    // Maybe add padding to allow data reuse with larger operator structs
    char *data_ptr = ((char *)cache + op->data_offset);
    if(op->file.size % 4096 == 0){
        op->bytes_cached = delilah_file_direct_read((char *)data_ptr, op->file.size, op->file.filename);
    } else {
        uint64_t aligned = op->file.size & ~0xFFF; // Align to 4KB
        op->bytes_cached = delilah_file_direct_read((char *)data_ptr, aligned, op->file.filename);
        op->bytes_cached += delilah_file_read_offset((char *)data_ptr + aligned, op->file.size - aligned, aligned, op->file.filename);
    }
    
    return 0x0;
}

