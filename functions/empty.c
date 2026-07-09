#include "../include/verified_functions.h"

struct layout {
    int dummy;
    char dynamic[];
};

int roger_prog(void *context) {
    struct layout *info = (struct layout *)context;
    struct req *reqs;
    struct mem_range *resources = obtain(reqs, 0);
    return 0x0;
}