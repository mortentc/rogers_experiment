#include "../include/verified_functions.h"

struct layout {
    int dates_offset;
    uint32_t low;
    uint32_t high;
    int result_start;
    int result_end;
    char dynamic[];
};

int roger_prog(void *context) {

    struct layout *info = (struct layout *)context;
    struct req *reqs;
    struct mem_range *resources = obtain(reqs, 0);

    struct mem_range dates;
    dates.ptr = info->dynamic;
    dates.length = info->dates_offset;

    struct mem_range converted_dates;
    converted_dates.ptr = dates.ptr + dates.length;
    convert(dates, TO_YEAR, &converted_dates);

    struct mem_range filtered_dates;
    filtered_dates.ptr = converted_dates.ptr + converted_dates.length;
    filter_uint32(converted_dates, &filtered_dates, info->low, info->high);

    info->result_start = info->dates_offset + converted_dates.length;
    info->result_end = info->result_start + filtered_dates.length;
    return 0x0;
}