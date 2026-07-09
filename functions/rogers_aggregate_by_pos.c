#include "../../../include/verified_functions.h"

struct layout {
    int element_size;
    int index_size;
    int result;
    char dynamic[];
};

// static inline int aggregate_uint32(struct mem_range elems, struct mem_range indices) {
//     return
//     aggregate_by_pos(
//         (int*) elems.ptr,
//         (uint32_t)elems.length / sizeof(int),
//         (uint32_t*) indices.ptr,
//         (uint32_t)indices.length / sizeof(uint32_t)
//     );
// }

int roger_prog(void *context) {
    struct layout *info = (struct layout *)context;
    struct req *reqs;
    struct mem_range *resources = obtain(reqs, 0);

    struct mem_range elements;
    elements.ptr = info->dynamic;
    elements.length = info->element_size;

    struct mem_range indices;
    indices.ptr = info->dynamic+elements.length;
    indices.length = info->index_size;

    info->result = aggregate_uint32(elements, indices);
    return 0x0;
}