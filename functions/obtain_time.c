#include "../../../include/verified_functions.h"
#include <time.h>

struct layout {
    int base_req;
    clock_t dummy;
    char dynamic[];
};

int roger_prog(void *context) {
    struct layout *info = (struct layout *)context;
    struct req reqs[1];
    reqs[0].id = info->base_req; reqs[0].p = Read;
    struct mem_range *resources = obtain(reqs, 1);
    info->dummy = clock();

    return 0x0;
}