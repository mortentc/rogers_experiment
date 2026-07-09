#include "../../../include/verified_functions.h"

struct layout {
    int element_req;
    int index_req;
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
    struct req reqs[2];
    reqs[0].id = info->element_req; reqs[0].p = Read;
    reqs[1].id = info->index_req;   reqs[0].p = Read;
    struct mem_range *resources = obtain(reqs, 2);

    info->result = aggregate_uint32(resources[0], resources[1]);
    return 0x0;
}