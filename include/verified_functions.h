#ifndef VERIF_FUNCS_H
#define VERIF_FUNCS_H

#include <stdint.h>
#include <stdlib.h>
#include "delilah.h"

typedef struct mem_range { char * ptr ; int length ; } mem_rng;
typedef enum perm { Read , Write } permission;
typedef struct req { int id ; permission p; } request;

mem_rng *obtain(request* requests, int len);

int aggregate_by_pos(int *data, uint32_t data_size, uint32_t *indices, uint32_t index_count);

void
filter(uint32_t* data, size_t len,
       uint32_t* out, size_t* written,
       uint32_t _c_pred1, uint32_t _c_pred2);


void convert_dates(
  char *date_strings, uint32_t bytes,
  CONVERSION_TYPE conv,
  uint32_t* results, uint32_t *res_length
);

static inline int aggregate_uint32(struct mem_range elems, struct mem_range indices) {
    return
    aggregate_by_pos(
        (int*) elems.ptr,
        (uint32_t)elems.length / sizeof(uint32_t),
        (uint32_t*) indices.ptr,
        (uint32_t)indices.length / sizeof(uint32_t)
    );
}

static inline void convert(
    struct mem_range dates,
    CONVERSION_TYPE conv,
    struct mem_range *results){
    convert_dates(
        dates.ptr, dates.length, conv,
        (uint32_t*)(results->ptr), (uint32_t*) &(results->length)
    );
    results->length *= sizeof(uint32_t);
}

static inline void filter_uint32(
    struct mem_range data,
    struct mem_range *result,
    uint32_t _c_pred1, uint32_t _c_pred2) {
    filter(
        (uint32_t*) data.ptr,
        (size_t) data.length / sizeof(size_t),
        (uint32_t*) result->ptr,
        (size_t*) &result->length,
        _c_pred1, _c_pred2
    );
    result->length *= sizeof(size_t);
}

#endif // VERIF_FUNCS_H