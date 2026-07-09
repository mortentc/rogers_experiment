#include <stdint.h>
#include <stdlib.h>

/*@
  axiomatic Occurrences{
    logic integer occurrences_in{L1}(uint32_t* arr, integer len, uint32_t value)
      reads arr[0..len-1];

    axiom empty: \forall uint32_t* arr, uint32_t v, integer len;
      len <= 0 ==> occurrences_in(arr, len, v) == 0;

    axiom in_nonempty:
      \forall uint32_t* arr, integer len, uint32_t v; len > 0 ==>
        (arr[len-1] == v <==>
        occurrences_in(arr, len, v) ==
        1 + occurrences_in(arr, len-1, v));

    axiom notin_nonempty:
      \forall uint32_t* arr, integer len, uint32_t v;
      len > 0 ==>
        (arr[len-1] != v <==>
        occurrences_in(arr, len, v) == occurrences_in(arr, len-1, v));

    axiom occurrence_size:
      \forall uint32_t* arr, integer len, uint32_t v; len > 0 ==>
      (\exists integer k; k == occurrences_in(arr, len, v)) ==>
      \forall integer i; 0 <= i < len ==> 0 <= arr[i] <= UINT32_MAX;
  }
*/

/*@
  predicate unchanged{L1, L2}(uint32_t* array, integer len) =
    \forall integer i ; 0 <= i < len ==> \at(array[i], L1) == \at(array[i], L2);

  lemma unchanged_occurrences{L1, L2}:
    \forall uint32_t *arr, value, integer len;
    unchanged{L1, L2}(arr, len) ==>
    occurrences_in{L1}(arr, len, value) == occurrences_in{L2}(arr, len, value);
*/

/*@
  requires \valid(out+(0..len-1)) && \valid(written);
  requires \valid_read(data + (0..len-1));
  requires \separated(written, data+(0..len-1), out+(0..len-1));

  assigns *written, out[0..len-1];

  ensures Correct:
    \forall size_t j;
    0 <= j < *written ==> _c_pred1 <= out[j] <= _c_pred2;

  ensures Complete:
    \forall uint32_t value;
    _c_pred1 <= value <= _c_pred2 ==>
      occurrences_in(data, len, value) ==
      occurrences_in(out, *written, value);
*/
void
filter(uint32_t* data, size_t len,
       uint32_t* out, size_t* written,
       uint32_t _c_pred1, uint32_t _c_pred2)
{
  *written = 0;
  /*@
    loop invariant 0 <= i <= len;
    loop invariant 0 <= *written <= i;
    loop invariant
      \forall size_t j;
      0 <= j < *written ==> _c_pred1 <= out[j] <= _c_pred2;

    loop invariant
      \forall uint32_t value;
      _c_pred1 <= value <= _c_pred2 ==>
      occurrences_in(data, i, value) == occurrences_in(out, *written, value);

    loop assigns i, *written, out[0..len-1];

    loop variant len-i;
  */
  for (size_t i = 0; i < len; i++) {
    const uint32_t inc = (data[i] >= _c_pred1) && (data[i] <= _c_pred2);
    out[*written] = data[i];
    *written += inc;
    //@ assert unchanged{Here, LoopCurrent}(data, i);
    /*@ ghost
      if((data[i] >= _c_pred1) && (data[i] <= _c_pred2)){
        //@ assert unchanged{Here, LoopCurrent}(out, *written-1);
        /@ assert \forall uint32_t value; _c_pred1 <= value <= _c_pred2 ==>
            occurrences_in(data, i, value) ==
            occurrences_in(out, *written-1, value); @/

      } else {
        //@ assert unchanged{Here, LoopCurrent}(out, *written);
        /@ assert \forall uint32_t value; _c_pred1 <= value <= _c_pred2 ==>
            occurrences_in(data, i, value) ==
            occurrences_in(out, *written, value); @/
      } */
  }
}