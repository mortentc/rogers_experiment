#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"

int main(){
    // Setup Ubpf VM
    struct ubpf_vm* vm = ubpf_create();
    int res = ubpf_register(vm, 1, "aggregate_by_pos", &aggregate_by_pos);
    printf("Registering code: %s\n", res ? "failed" : "succeeded");
    // Load ebpf code
    FILE *fptr = fopen("programs/test.o", "rb");
    fseek(fptr, 0L, SEEK_END);
    int sz = ftell(fptr);
    rewind(fptr);
    char *ebpf = (char*) malloc(sz);
    fread(ebpf, 1, sz, fptr);
    char *err = malloc(128);
    ubpf_load_elf(vm, ebpf, sz, &err);
    if(err) printf("errors: %s\n", err);
    // Prepare data
    struct layout {
        int element_size;
        int index_size;
        int result;
        char dynamic[];
    };
    int dummy_data[] = {1,2,3,0,2};
    int data_size = sizeof(struct layout)+sizeof(dummy_data);
    struct layout *input = (struct layout*)malloc(1024);
    input -> element_size = 3*sizeof(int);
    input -> index_size = 2*sizeof(int);
    void *data = input->dynamic;
    memcpy(input->dynamic, dummy_data, sizeof(dummy_data));
    // Execute program
    uint64_t result;
    void *shared = malloc(1024);
    printf("Before crash\n");
    ubpf_exec(vm, input, 1024, &result, shared, 1024);
    printf("Execution result: %ld\n", result);
    // Clean up
    free(shared);
    free(input);
    free(ebpf);
    free(err);
    ubpf_destroy(vm);
}