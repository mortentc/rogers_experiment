
#include "../include/delilah.h"

int prog(void *ctx, int mem_size) {
    filter_count_op * op = (filter_count_op *)ctx;
    
    //delilah_file_read(ctx + sizeof(filter_count_op), op->file.size, op->file.filename);

    uint64_t element_count = op->file.size / sizeof(uint32_t);
    uint64_t result = 0;
    uint32_t const * ptr = ctx + sizeof(filter_count_op);
    uint32_t const * const end = ptr + element_count;

    // Assuming that branching is expensive, we only branch in the beginning and manually roll out everything else.
    if(op->comp_type == EQ) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr == comp0) ? 1:0;
        }
    } else if(op->comp_type == NEQ) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr == comp0) ? 0:1;
        }
    } else if(op->comp_type == LT) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr < comp0) ? 1:0;
        }
    } else if(op->comp_type == LE) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr <= comp0) ? 1:0;
        }
    } else if(op->comp_type == GT) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr > comp0) ? 1:0;
        }
    } else if(op->comp_type == GE) {
        uint32_t const comp0 = op->comp0;
        for(; ptr != end; ++ptr) {
            result += (*ptr >= comp0) ? 1:0;
        }
    } else if(op->comp_type == BW) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        for(; ptr != end; ++ptr) {
            result += ((*ptr > comp0) && (*ptr < comp1)) ? 1:0;
        }
    } else if(op->comp_type == BWI) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        for(; ptr != end; ++ptr) {
            result += ((*ptr >= comp0) && (*ptr <= comp1)) ? 1:0;
        }
    } else if(op->comp_type == BWLI) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        for(; ptr != end; ++ptr) {
            result += ((*ptr >= comp0) && (*ptr < comp1)) ? 1:0;
        }
    } else if(op->comp_type == BWHI) {
        uint32_t const comp0 = op->comp0;
        uint32_t const comp1 = op->comp1;
        for(; ptr != end; ++ptr) {
            result += ((*ptr > comp0) && (*ptr <= comp1)) ? 1:0;
        }
    }
        
    op->result = result;
    return 0x0;
}


 
