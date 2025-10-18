#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "resource_manager.c"

#define DATA_SIZE 1000000
#define RUNS 100
#define WARMUP 10

void dummy(void){return;}
mem_rng *obtain(request *reqs, int len){
    if(len == 0) return NULL;
    obtain_triplet *triplet = malloc(sizeof(obtain_triplet));
    mem_rng *res = malloc(len * sizeof(mem_rng));
    request_set *set = malloc(sizeof(request_set));
    set->reqs = reqs;
    set->count = len;
    triplet->reqs = *set;
    triplet->res = res;
    int flag = 0; triplet->flag = &flag;
    enqueue_req(&arrivals, *triplet);
    // printf("Waiting\n");
    while(!flag){/* wait */}

    // printf("Request %s\n", flag == 1 ? "successful" : "failed");
    free(triplet);
    free(set);
    // printf("Length of first region: %d\n", res[0].length);
    return res;
};
// mock implementation to return data size
// uint64_t delilah_file_read_offset(char *a, uint64_t b, uint64_t c, char* d){ return DATA_SIZE; }

#define PRINT_TO_TERMINAL true

int main(){
    // Setup Ubpf VM
    struct ubpf_vm* vm = ubpf_create();
    pthread_t *manager = init_manager();
    ubpf_register(vm, 1, "obtain", &obtain);
    ubpf_register(vm, 2, "aggregate_by_pos", &aggregate_by_pos);
    ubpf_register(vm, 3, "convert_dates", &convert_dates);
    ubpf_register(vm, 4, "filter", &filter);

    struct layout {
        int base_req;
        char dynamic[];
    };
    int data_sz = sizeof(struct layout);
    struct layout *input = (struct layout*)malloc(data_sz);
    mem_rng dummy;
    int element_id = allocate_shared(sizeof(int) * DATA_SIZE, &dummy);
    request imm[1] = {{element_id, Write}};
    request_set immediate_release;
    immediate_release.reqs = imm;
    immediate_release.count = 1;
    enqueue_rel(&releases, immediate_release);
    input->base_req = element_id;

    // Load ebpf code
    FILE *fptr = fopen("programs/empty.o", "rb");
    fseek(fptr, 0L, SEEK_END);
    int sz = ftell(fptr);
    rewind(fptr);
    char *ebpf = (char*) malloc(sz);
    fread(ebpf, 1, sz, fptr);
    char *err = malloc(1024);
    ubpf_load_elf(vm, ebpf, sz, &err);
    if(err) printf("errors: %s\n", err);

    // Declare time-keeping variables
    FILE *log = fopen("data/manager.txt", "w");
    char out[10];
    uint64_t result;
    clock_t start, finish;
    clock_t empty_time = 0, aggr_time = 0;
    double avg_time = 0;
    
    // Execute empty CSF
    // Warmup
    for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        empty_time += finish - start;
    }
    avg_time = ((double)empty_time)/RUNS;
    // sprintf(out, "%.3f\n", avg_time);
    // fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Empty]: %.3f µs\n", avg_time);

    // Clean up
    ubpf_unload_code(vm);
    free(ebpf);
    free(err);

    fptr = fopen("programs/obtain_1.o", "rb");
    fseek(fptr, 0L, SEEK_END);
    sz = ftell(fptr);
    rewind(fptr);
    ebpf = (char*) malloc(sz);
    fread(ebpf, 1, sz, fptr);
    err = malloc(1024);
    ubpf_load_elf(vm, ebpf, sz, &err);
    if(err) printf("errors: %s\n", err);

    clock_t release_time = 0;
    request_set dummy_reqs;
    request reqs[1] = {{element_id, Read}};
    dummy_reqs.reqs = reqs;
    dummy_reqs.count = 1;
    // Execute CSF obtaining 1 shared region
    // Warmup
    for(int i = 0; i<WARMUP; i++){
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        enqueue_rel(&releases, dummy_reqs);
    }
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        aggr_time += finish - start;

        start = clock();
        enqueue_rel(&releases, dummy_reqs);
        finish = clock();
        release_time += finish - start;
    }
    avg_time = ((double)(aggr_time-empty_time))/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Obtain1]: %.3f µs\n", avg_time);

    avg_time = ((double)release_time)/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Release1]: %.3f µs\n", avg_time);

    aggr_time = 0;
    mem_rng *mock = (mem_rng*)malloc(sizeof(mem_rng));
    for(int i = 0; i<WARMUP; i++) allocate_shared(10, mock);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        allocate_shared(10, mock);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Alloc]: %.3f µs\n", avg_time);

    // Clean up
    ubpf_unload_code(vm);
    free(input);
    free(ebpf);
    free(err);
    destroy_manager(manager);
}