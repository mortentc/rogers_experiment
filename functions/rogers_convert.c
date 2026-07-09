#include "../include/verified_functions.h"

struct layout {
    int dates_offset;
    CONVERSION_TYPE conv;
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
    convert(dates, info->conv, &converted_dates);

    return 0x0;
}