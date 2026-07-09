#include "../include/verified_functions.h"

struct layout {
    int base_req;
    char dynamic[];
};

int rogers_prog(void *context) {
    struct layout *info = (struct layout *)context;
    struct req reqs[1];
    reqs[0].id = info->base_req; reqs[0].p = Read;
    struct mem_range *resources = obtain(reqs, 1);
    return 0x0;
}