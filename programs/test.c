#include "../external/delilah/include/verified_functions.h"
int prog(void* ctx){
    int data[] = {1,2,3};
    uint32_t idx[] = {0,2};
    aggregate_by_pos(data, 3, idx, 2);
    return 28;
}