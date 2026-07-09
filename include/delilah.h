#ifndef DELILAH_H
#define DELILAH_H

#define CHUNK_SIZE (128 * 1024 * 1024)

#include <stdint.h> // only included for local benchmarking
/*
# ifndef __uint8_t_defined
# define __uint8_t_defined
*/
//typedef unsigned char uint8_t;
/*
# endif
# ifndef __uint32_t_defined
# define __uint32_t_defined
*/
//typedef unsigned int uint32_t;
/*
# endif
# ifndef __uint64_t_defined
# define __uint64_t_defined
*/
//typedef unsigned long long uint64_t;
/*
# endif
*/

#include <stdbool.h>

struct delilah_file_t {
    uint64_t size;
    char filename[128];
};

struct delilah_io_state_t {
    uint64_t read;
};

/* DB-Operator related struct BEGIN */

typedef enum {
    EQ,
    NEQ,
    LT,
    LE,
    GT,
    GE,
    BW /*BETWEEN:                  (comp0,comp1)*/,
    BWI /*BETWEEN inclusive:        [comp0,comp1]*/,
    BWLI /*BETWEEN low inclusive:    [comp0,comp1)*/,
    BWHI /*BETWEEN high inclusive:   (comp0,comp1]*/
} COMP_TYPE;

typedef enum { TO_YEAR,
               TO_MONTH,
               TO_DAY } CONVERSION_TYPE;

typedef struct {
    COMP_TYPE comp_type;
    uint32_t comp0;
    uint32_t comp1;
    struct delilah_file_t file;
    uint64_t result;
} filter_count_op;

typedef struct {
    COMP_TYPE comp_type;
    uint32_t comp0;
    uint32_t comp1;
    uint32_t result_count;
    uint32_t result_offset;
    uint32_t padding_offset;
    struct delilah_file_t file;
    bool inplace;
    bool reuse_data;
    bool use_cache;
    uint32_t cache_offset;
    uint32_t cached_data_offset;
} filter_op;

typedef struct {
    COMP_TYPE comp_type;
    uint32_t comp0;
    uint32_t comp1;
    uint32_t result_count;
    uint32_t result_offset;
    uint32_t padding_offset;
    struct delilah_file_t file;
    bool inplace;
    bool reuse_data;
    bool use_cache;
    uint32_t cache_offset;
    uint32_t cached_data_offset;
    uint8_t eng;
} filter_op_hw;

typedef struct {
    CONVERSION_TYPE conv_type;
    uint32_t padding_offset;
    struct delilah_file_t file;
    uint32_t result_count;
    uint32_t result_offset;
    uint32_t cache_offset;
    uint32_t cached_data_offset;
    bool inplace;
    bool reuse_data;
    bool use_cache;
} conversion_op;

typedef struct {
    CONVERSION_TYPE conv_type;
    COMP_TYPE comp_type;
    uint32_t comp0;
    uint32_t comp1;
    uint32_t padding_offset;
    struct delilah_file_t file;
    uint32_t result_count;
    uint32_t result_offset;
    uint32_t cache_offset;
    uint32_t cached_data_offset;
    bool reuse_data;
    bool use_cache;
} fused_filter_conversion_op;

typedef struct {
    uint32_t padding_offset;
    struct delilah_file_t file;
    uint32_t available_positions_count;
    uint32_t result_count;
    uint32_t result_offset;
    uint32_t cache_offset;
    uint32_t cached_data_offset;
    bool use_cache;
} aggregation_by_pos_op;

typedef struct {
    uint32_t padding_offset;
    struct delilah_file_t file;
    uint32_t data_offset;
    uint32_t result_count;
    uint32_t result_offset;
    bool use_cache;
} read_file_op;

typedef struct {
    struct delilah_file_t file;
    uint32_t data_offset;
    uint32_t bytes_cached;
    bool use_cache;
} cache_file_op;

typedef struct {
    uint32_t bytes;
    uint32_t padding_offset;
    uint32_t data_offset;
    uint32_t result;
    bool use_cache;
} memory_bench_op;

typedef char bitmask_t;

/* DB-Operator related struct END */

#endif  // DELILAH_H
