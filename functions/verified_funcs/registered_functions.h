#ifndef REGFUNCS_H
#define REGFUNCS_H

#include <stdint.h>
#include <stdlib.h>
int aggregate_by_pos(int *data, uint32_t data_size, uint32_t *indices, uint32_t index_count);

void
delilah_functions_tsl_filter_sequential(uint32_t* data, size_t len,
                                        uint32_t* out, size_t* written,
                                        uint32_t _c_pred1, uint32_t _c_pred2);
typedef enum {
  TO_YEAR,
  TO_MONTH,
  TO_DAY } CONVERSION_TYPE;

void convert_dates(
  char *date_strings, uint32_t bytes,
  CONVERSION_TYPE conv,
  uint32_t* results, uint32_t *res_length
);

#endif // REGFUNCS_H