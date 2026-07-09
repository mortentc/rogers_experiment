#include <stdint.h>

#define TUPLE_WIDTH 8

typedef enum {
  TO_YEAR,
  TO_MONTH,
  TO_DAY } CONVERSION_TYPE;

/*@ 
  predicate is_digits(char *string, integer length) =
    \forall integer i; 0 <= i < length ==> '0' <= string[i] <= '9';

  axiomatic Conversion{
    logic uint32_t int_of_string(char *string, integer length);

    axiom no_digit:
      \forall char *string; int_of_string(string, 0) == 0;

    axiom multi_digit:
      \forall char *string, integer length;
      0 <= length ==>
      int_of_string(string, length+1) ==
      int_of_string(string, length) * 10 + string[length] - '0';
  }
*/

/*@
  requires 0 <= len;
  requires \valid_read(string+(0..len-1));
  requires is_digits(string, len);

  assigns \nothing;

  ensures \result == int_of_string(string, len);
*/
static inline uint32_t parse_string(char *string, int len){
  uint32_t res = 0;
  /*@
    loop invariant res == int_of_string(string, i);
    loop invariant 0 <= i <= len;
    loop assigns i, res;
    loop variant len - i;
  */
  for(int i = 0; i<len; i++) res = string[i] - '0' + res * 10;
  return res;
}

/*@
  lemma substring_digits:
    \forall char* string, integer length;
    is_digits(string, length) ==>
    \forall integer i; 0 <= i < length ==>
    is_digits(string+i, length-i);
*/

// SPECIFICATION
/*@
  requires \separated(
    date_strings+(0..bytes-1),
    results+(0..bytes/TUPLE_WIDTH-1),
    res_length
  );
  requires \valid(results+(0..bytes/TUPLE_WIDTH-1)) && \valid(res_length);
  requires \valid_read(date_strings+(0..bytes-1));

  requires is_digits(date_strings, bytes);

  requires bytes % TUPLE_WIDTH == 0;

  assigns *res_length, results[0..bytes/TUPLE_WIDTH-1];

  behavior to_year:
    assumes conv == TO_YEAR;
    ensures \forall integer idx;
      0 <= idx < bytes / TUPLE_WIDTH ==>
      results[idx] == int_of_string(date_strings+idx*TUPLE_WIDTH, 4);

  behavior to_month:
    assumes conv == TO_MONTH;
    ensures \forall integer idx;
      0 <= idx < bytes / TUPLE_WIDTH ==>
      results[idx] == int_of_string(date_strings+idx*TUPLE_WIDTH+4, 2);

  behavior to_day:
    assumes conv == TO_DAY;
    ensures \forall integer idx;
      0 <= idx < bytes / TUPLE_WIDTH ==>
      results[idx] == int_of_string(date_strings+idx*TUPLE_WIDTH+6, 2);
*/
void _convert_dates(
  char *date_strings, uint32_t bytes,
  CONVERSION_TYPE conv,
  uint32_t* results, uint32_t *res_length
){
  uint32_t num_results = 0;
  char *data_ptr_end = date_strings + bytes;
  //@ ghost int entries = bytes / TUPLE_WIDTH;
  //@ ghost int remaining = bytes / TUPLE_WIDTH;
  if (conv == TO_YEAR) {
    /*@
      loop invariant remaining >= 0;
      loop invariant num_results + remaining == entries;
      loop invariant date_strings + remaining * TUPLE_WIDTH == data_ptr_end;

      loop invariant
        \forall integer i;
        0 <= i < num_results ==>
        results[i] == int_of_string(\at(date_strings, Pre)+i*TUPLE_WIDTH, 4);

      loop invariant is_digits(date_strings, remaining * TUPLE_WIDTH);

      loop assigns remaining, results[0..entries-1], num_results, date_strings;

      loop variant remaining;
    */
    for (; date_strings != data_ptr_end; date_strings += TUPLE_WIDTH) {
      results[num_results++] = parse_string(date_strings, 4);
      //@ ghost remaining--;
    }
  } else if (conv == TO_MONTH) {
    /*@
      loop invariant remaining >= 0;
      loop invariant num_results + remaining == entries;
      loop invariant date_strings + remaining * TUPLE_WIDTH == data_ptr_end;

      loop invariant
        \forall integer i;
        0 <= i < num_results ==>
        results[i] == int_of_string(\at(date_strings, Pre)+i*TUPLE_WIDTH+4, 2);

      loop invariant is_digits(date_strings, remaining * TUPLE_WIDTH);

      loop assigns remaining, results[0..entries-1], num_results, date_strings;

      loop variant remaining;
    */
    for (; date_strings != data_ptr_end; date_strings += TUPLE_WIDTH) {
      results[num_results++] = parse_string(date_strings+4, 2);
      //@ ghost remaining--;
    }
  } else if (conv == TO_DAY) {
    /*@
      loop invariant remaining >= 0;
      loop invariant num_results + remaining == entries;
      loop invariant date_strings + remaining * TUPLE_WIDTH == data_ptr_end;

      loop invariant
        \forall integer i;
        0 <= i < num_results ==>
        results[i] == int_of_string(\at(date_strings, Pre)+i*TUPLE_WIDTH+6, 2);

      loop invariant is_digits(date_strings, remaining * TUPLE_WIDTH);

      loop assigns remaining, results[0..entries-1], num_results, date_strings;

      loop variant remaining;
    */
    for (; date_strings != data_ptr_end; date_strings += TUPLE_WIDTH) {
      results[num_results++] = parse_string(date_strings+6, 2);
      //@ ghost remaining--;
    }
  }
  *res_length = num_results;
}