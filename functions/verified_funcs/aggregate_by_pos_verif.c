#include <stdint.h>

/*@
  axiomatic Sum_array{
    logic integer sum(int* array, integer len) reads array[0 .. (len-1)];

    axiom empty:
      \forall int* a, integer l; 0 >= l ==> sum(a,l) == 0;
      
    axiom range:
      \forall int* a, integer l; 0 < l ==> sum(a,l) == sum(a,l-1)+a[l-1];
  }
*/

/*@ ghost
  /@
    requires \valid_read(src+(0..src_len-1));
    requires \valid_read(indices+(0..idx_len-1));
    requires \valid(dest+(0..idx_len-1));

    requires \separated(
      src+(0..src_len-1),
      indices+(0..idx_len-1),
      dest+(0..idx_len-1)
    );

    requires IN_BOUNDS:
      \forall integer idx;
      0 <= idx < idx_len ==> 0 <= indices[idx] < src_len;

    assigns dest[0..idx_len-1];

    ensures TOTAL:
      \forall integer idx;
      0 <= idx < idx_len ==> dest[idx] == src[indices[idx]];
  @/
  void select(
    int *src, uint32_t src_len,
    uint32_t *indices, uint32_t idx_len,
    int \ghost * dest
  ){
    /@
      loop invariant COMPLETE:
        \forall integer idx;
        0 <= idx < i ==> dest[idx] == src[indices[idx]];

      loop assigns i, dest[0..idx_len-1];

      loop variant idx_len - i;
    @/
    for(uint32_t i = 0; i<idx_len; i++) dest[i] = src[indices[i]];
} */

/*@
  requires \valid_read(indices+(0..index_count-1));
  requires \valid_read(data+(0..data_size-1));
  requires \valid(terms+(0..index_count-1));

  requires \separated(
    indices+(0..index_count-1),
    data+(0..data_size-1),
    terms+(0..index_count-1)
  );

  requires IN_BOUNDS:
    \forall integer idx;
    0 <= idx < index_count ==> 0 <= indices[idx] < data_size;

  assigns terms[0..index_count-1];

  ensures TOTAL:
    \forall integer idx;
    0 <= idx < index_count ==> terms[idx] == data[indices[idx]];

  ensures AGGREGATE: \result == sum(terms, index_count);
*/
int aggregate_by_pos(
  int *data, uint32_t data_size,
  uint32_t *indices, uint32_t index_count
) /*@ ghost (int terms[])*/ {
  int result = 0;
  uint32_t *indices_end = indices + index_count;
  //@ ghost select(data, data_size, indices, index_count, terms);
  //@ ghost uint32_t ghost_count = 0;
  /*@
    loop invariant indices + index_count - ghost_count == indices_end;
    loop invariant 0 <= ghost_count <= index_count;
    
    loop invariant SUM: result == sum(terms, ghost_count);

    loop assigns result, indices, ghost_count;
    loop variant index_count - ghost_count;
  */
  for (; indices != indices_end; ++indices) {
    result += data[*indices];
    //@ ghost ghost_count++;
  }
  return result;
}