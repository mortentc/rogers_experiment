#include <stdint.h>

typedef enum { EQ, BWI} COMPARISON;

/*@
  predicate unchanged{L1, L2}(uint32_t* ptr, integer len) =
    \forall integer i ; 0 <= i < len ==>
    \at(ptr[i], L1) == \at(ptr[i], L2);
*/

/*@
  requires \valid_read(elements+(0..element_count-1));
  requires \valid(indices+(0..element_count-1));

  requires \separated(
    res_size,
    elements+(0..element_count-1),
    indices+(0..element_count-1)
  );

  assigns *res_size, indices[0..element_count-1];
*/
void filter_tids(uint32_t *elements, uint32_t element_count, uint32_t *indices, uint32_t *res_size, COMPARISON comp_type, uint32_t comp0, uint32_t comp1){
  uint32_t tuple_idx = 0;
  uint32_t *elements_end = elements + element_count;
  uint32_t *indices_start = indices;
  //@ ghost uint32_t matches = 0;
  //@ ghost uint32_t *elements_start = elements;
  if (comp_type == EQ) {
    /*@
      loop invariant 0 <= matches <= tuple_idx <= element_count;
      loop invariant elements + element_count - tuple_idx == elements_end;
      loop invariant indices == indices_start + matches;
      loop invariant elements == elements_start + tuple_idx;

      loop invariant CORRECT:
        \forall integer i;
        0 <= i < matches ==>
        elements_start[indices_start[i]] == comp0;

      loop assigns elements, tuple_idx, matches, indices, indices_start[0..element_count-1];
      loop variant element_count - tuple_idx;
    */
    for (; elements != elements_end; ++elements, ++tuple_idx) {
      const uint32_t inc = (*elements == comp0) ? 1 : 0;
      *indices = tuple_idx;
      indices += inc;
      /*@ ghost
      matches += inc;
      */
    }
  } else if (comp_type == BWI) {
    /*@
      loop invariant 0 <= matches <= tuple_idx <= element_count;
      loop invariant elements + element_count - tuple_idx == elements_end;
      loop invariant indices == indices_start + matches;
      loop invariant elements == elements_start + tuple_idx;

      loop assigns elements, tuple_idx, matches, indices, indices_start[0..element_count-1];
      loop variant element_count - tuple_idx;
    */
    for (; elements != elements_end; ++elements, ++tuple_idx) {
      const uint32_t inc = ((*elements >= comp0) && (*elements <= comp1)) ? 1 : 0;
      *indices = tuple_idx;
      indices += inc;
      //@ ghost matches += inc;
    }
  }
  *res_size = indices - indices_start;
}