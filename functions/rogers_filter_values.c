#include "../include/verified_functions.h"

struct layout {
    int elements_offset;
    uint32_t low;
    uint32_t high;
    int result_length;
    char dynamic[];
};

int roger_prog(void *context) {

    struct layout *info = (struct layout *)context;
    struct req *reqs;
    struct mem_range *resources = obtain(reqs, 0);

    struct mem_range elements;
    elements.ptr = info->dynamic;
    elements.length = info->elements_offset;

    struct mem_range filtered_elements;
    filtered_elements.ptr = elements.ptr + elements.length;
    filter_uint32(elements, &filtered_elements, info->low, info->high);
    info->result_length = filtered_elements.length;

    return 0x0;
}