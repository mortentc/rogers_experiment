#include <stdio.h>
#include "ubpf.h"
#include "verified_functions.h"

int main(){
    struct ubpf_vm* vm = ubpf_create();
    int res = ubpf_register(vm, 0, "aggregate_by_pos", &aggregate_by_pos);
    printf("Return code: %d\n", res);
}