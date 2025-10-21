#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "external/delilah/delilah/programs/convert_date.c"
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

    // Load ebpf code
    FILE *fptr = fopen("programs/rogers_convert.o", "rb");
    fseek(fptr, 0L, SEEK_END);
    int sz = ftell(fptr);
    rewind(fptr);
    char *ebpf = (char*) malloc(sz);
    fread(ebpf, 1, sz, fptr);
    char *err = malloc(1024);
    ubpf_load_elf(vm, ebpf, sz, &err);
    if(err) printf("errors: %s\n", err);

    // Prepare data
    struct layout {
        int dates_offset;
        CONVERSION_TYPE conv;
        char dynamic[];
    };
    int data_sz =
        sizeof(struct layout) + DATA_SIZE * 4 +
        DATA_SIZE * sizeof(int) / 2;
    struct layout *input = (struct layout*)malloc(data_sz);
    input->dates_offset = DATA_SIZE * 4;
    input->conv = TO_YEAR;
    FILE *data = fopen("data/dates.dat", "rb");
    fread(input->dynamic, 1, DATA_SIZE * 4, data);
    fclose(data);

    // Declare time-keeping variables
    FILE *log = fopen("data/verified.txt", "w");
    char out[10];
    uint64_t result;
    clock_t start, finish;
    clock_t aggr_time = 0;
    double avg_time = 0;
    
    // Execute interpreted CSF with compiled and verified sum
    // Warmup
    for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_YEAR]: %.3f ms\n", avg_time);

    conversion_op *original_input = (conversion_op*)malloc(sizeof(conversion_op)+DATA_SIZE*4);
    memcpy(original_input+1, input->dynamic, DATA_SIZE*4);
    original_input->use_cache = false;
    original_input->reuse_data = true;
    original_input->padding_offset = sizeof(conversion_op);
    original_input->file.size = DATA_SIZE*4;
    original_input->inplace = true;
    original_input->conv_type = TO_YEAR;

    aggr_time = 0;
    for(int i = 0; i<WARMUP; i++) prog(original_input, 0, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        prog(original_input, 0, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_YEAR][OLD]: %.3f ms\n", avg_time);

    // Execute interpreted CSF with compiled and verified sum
    input->conv = TO_MONTH;
    aggr_time = 0;
    // Warmup
    for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_MONTH]: %.3f ms\n", avg_time);

    original_input->conv_type = TO_MONTH;
    aggr_time = 0;
    for(int i = 0; i<WARMUP; i++) prog(original_input, 0, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        prog(original_input, 0, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_MONTH][OLD]: %.3f ms\n", avg_time);

    // Execute interpreted CSF with compiled and verified sum
    input->conv = TO_DAY;
    aggr_time = 0;
    // Warmup
    for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_DAY]: %.3f ms\n", avg_time);

    original_input->conv_type = TO_DAY;
    aggr_time = 0;
    for(int i = 0; i<WARMUP; i++) prog(original_input, 0, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        prog(original_input, 0, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [TO_DAY][OLD]: %.3f ms\n", avg_time);

    // Clean up
    ubpf_unload_code(vm);
    free(input);
    free(ebpf);
    free(err);
    free(original_input);
}