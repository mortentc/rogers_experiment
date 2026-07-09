#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "functions/filter_sequential.c"
#include "functions/rogers_filter_values.c"
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

    // Prepare data
    struct layout *input = (struct layout*)malloc(
        sizeof(struct layout) +
        sizeof(int) * DATA_SIZE * 2
    );
    input->elements_offset = DATA_SIZE*sizeof(int);
    input->low = 0;
    input->high = 2000;
    FILE *data = fopen("data/sum.dat", "rb");
    fread(input->dynamic, 4, DATA_SIZE, data);
    fclose(data);

    // Declare time-keeping variables
    FILE *log = fopen("data/extra.txt", "w");
    char out[10];
    uint64_t result;
    clock_t start, finish;
    clock_t aggr_time = 0;
    double avg_time = 0;

    // Execute compiled CSF with compiled and verified sum
    // Warmup
    for(int i = 0; i<WARMUP; i++) rogers_prog(input);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        rogers_prog(input);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [CompFilter]: %.3f ms\n", avg_time);

    filter_op *original = (filter_op*)malloc(
        sizeof(filter_op) + DATA_SIZE * sizeof(int) * 2
    );
    original->file.size = DATA_SIZE * sizeof(uint32_t);
    original->use_cache = false;
    original->inplace = false;
    original->padding_offset = sizeof(filter_op);
    original->comp_type = BWI;
    original->comp0 = 0;
    original->comp1 = 2000;
    memcpy(((char*)original)+sizeof(filter_op)+DATA_SIZE*sizeof(int), input->dynamic, DATA_SIZE*sizeof(int));

    aggr_time = 0;
    for(int i = 0; i<WARMUP; i++) prog(original, 0, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        prog(original, 0, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [CompFilter][OLD]: %.3f ms\n", avg_time);

    // Clean up
    ubpf_unload_code(vm);
}