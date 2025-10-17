#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "external/delilah/delilah/programs/rogers_programs/filter_dates.c"
#include "external/delilah/delilah/programs/filter_convert_date.c"
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

    int data_sz = sizeof(struct layout)+DATA_SIZE;
    struct layout *input = (struct layout*)malloc(
        sizeof(struct layout) + DATA_SIZE +
        DATA_SIZE * sizeof(int) * 2 / 8
    );
    input->dates_offset = DATA_SIZE;
    input->low = 1000;
    input->high = 2000;
    FILE *data = fopen("data/dates.dat", "rb");
    fread(input->dynamic, 1, DATA_SIZE, data);
    fclose(data);

    // Declare time-keeping variables
    FILE *log = fopen("data/composed.txt", "w");
    char out[10];
    uint64_t result;
    clock_t start, finish;
    clock_t aggr_time = 0;
    double avg_time = 0;
    
    // Execute interpreted CSF with compiled and verified sum
    // Warmup
    for(int i = 0; i<WARMUP; i++) roger_prog(input);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        roger_prog(input);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Composed]: %.3f ms\n", avg_time);

    
    uint32_t cache_size = sizeof(struct delilah_io_state_t) + DATA_SIZE;
    char *_cache = (char*)malloc(
        cache_size + DATA_SIZE/2 + sizeof(fused_filter_conversion_op)
    );
    fused_filter_conversion_op *original = (fused_filter_conversion_op*)(_cache+cache_size);
    original->padding_offset = sizeof(fused_filter_conversion_op);
    original->cached_data_offset = sizeof(struct delilah_io_state_t);
    original->file.size = DATA_SIZE;
    original->conv_type = TO_YEAR;
    original->comp_type = BWI;
    original->comp0 = 1000;
    original->comp1 = 2000;
    memcpy(_cache+sizeof(struct delilah_io_state_t), input->dynamic, DATA_SIZE);

    aggr_time = 0;
    for(int i = 0; i<WARMUP; i++) prog(original, 0, _cache, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        prog(original, 0, _cache, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Composed][OLD]: %.3f ms\n", avg_time);
    
    // Clean up
    ubpf_unload_code(vm);
    free(input);
    free(_cache);
    // free(ebpf);
    // free(err);
}