// #include "../external/delilah/include/verified_functions.h"
struct layout {
        int element_size;
        int index_size;
        int result;
        char dynamic[];
    };
int prog(void* ctx, int ctx_len, void* cache, int cache_len){
    struct layout *info = (struct layout *)ctx;
    return aggregate_by_pos(info->dynamic,3,info->dynamic+3*sizeof(int),2);
}