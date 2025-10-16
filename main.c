#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "external/delilah/delilah/programs/roger_programs/roger_aggregate_by_pos.c"
#include "external/delilah/delilah/programs/aggregate_by_pos.c"
#include "resource_manager.c"

#define DATA_SIZE 1000000
#define RUNS 10
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
    printf("Waiting\n");
    while(!flag){/* wait */}
    printf("Waiting done\n");
    free(triplet);
    printf("Length of first region: %d\n", res[0].length);
    return res;
};

#define PRINT_TO_TERMINAL true

int main(){
    // Setup Ubpf VM
    struct ubpf_vm* vm = ubpf_create();
    pthread_t *manager = init_manager();
    ubpf_register(vm, 1, "obtain", &obtain);
    ubpf_register(vm, 2, "aggregate_by_pos", &aggregate_by_pos);
    ubpf_register(vm, 3, "convert_dates", &convert_dates);
    ubpf_register(vm, 4, "filter", &filter);
    // Experiment 1: Baseline
    {
    
    // Load ebpf code
    FILE *fptr = fopen("programs/aggregate.o", "rb");
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
    int data_sz = sizeof(struct layout)+(DATA_SIZE*3/2)*sizeof(int);
    struct layout *input = (struct layout*)malloc(
        sizeof(struct layout) +
        sizeof(int) * DATA_SIZE +
        sizeof(uint32_t) * DATA_SIZE/2
    );
    input->element_size = DATA_SIZE*sizeof(int);
    input->index_size = DATA_SIZE/2*sizeof(uint32_t);
    FILE *data = fopen("data/sum.dat", "rb");
    fread(input->dynamic, 4, DATA_SIZE*3/2, data);
    fclose(data);

    // Declare time-keeping variables
    FILE *log = fopen("data/baseline.txt", "w");
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
    printf("Time taken [MixedVerif]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

    // Execute compiled CSF with compiled and verified sum
    aggr_time = 0;
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
    printf("Time taken [CompVerif]: %.3f ms\n", avg_time);

    // Previous approach
    {
        // Setup VM, Data and eBPF code
        struct ubpf_vm* vm = ubpf_create();
        ubpf_register(vm, 1, "delilah_file_read", &dummy);
        FILE *fptr = fopen("programs/old_aggregate.o", "rb");
        fseek(fptr, 0L, SEEK_END);
        int sz = ftell(fptr);
        rewind(fptr);
        char *ebpf = (char*) malloc(sz);
        fread(ebpf, 1, sz, fptr);
        char *err = malloc(128);
        ubpf_load_elf(vm, ebpf, sz, &err);
        
        if(err) printf("errors: %s\n", err);
        int data_sz = sizeof(aggregation_by_pos_op) + sizeof(uint32_t) * (DATA_SIZE/2+1);
        aggregation_by_pos_op *op = malloc(data_sz + DATA_SIZE*sizeof(int));
        op->padding_offset = sizeof(aggregation_by_pos_op);
        op->available_positions_count = DATA_SIZE/2;
        op->cached_data_offset = 0;
        op->use_cache = true;

        uint32_t *positions = ((uint32_t*)(op+1))+1;
        memcpy(positions, input->dynamic+(DATA_SIZE*sizeof(int)), DATA_SIZE/2*sizeof(uint32_t));
        void *shared = positions+DATA_SIZE/2;
        memcpy(shared, input->dynamic, DATA_SIZE*sizeof(int));

        // Previous approach fully interpreted
        // Warmup
        for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, op, data_sz, &result, shared, DATA_SIZE*sizeof(int));
        aggr_time = 0;
        for(int i = 0; i<RUNS; i++){
            start = clock();
            ubpf_exec(vm, op, data_sz, &result, shared, DATA_SIZE*sizeof(int));
            finish = clock();
            aggr_time += finish - start;
        }
        avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
        sprintf(out, "%.3f\n", avg_time);
        fwrite(out, strlen(out), 1, log);
        if(PRINT_TO_TERMINAL)
        printf("Time taken [VmOld]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

        // Previous approach fully compiled
        // Warmup
        for(int i = 0; i<WARMUP; i++) prog(op, data_sz, shared, DATA_SIZE*sizeof(int));
        aggr_time = 0;
        for(int i = 0; i<RUNS; i++){
            start = clock();
            prog(op, data_sz, shared, DATA_SIZE*sizeof(int));
            finish = clock();
            aggr_time += finish - start;
        }
        avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
        sprintf(out, "%.3f\n", avg_time);
        fwrite(out, strlen(out), 1, log);
        if(PRINT_TO_TERMINAL)
        printf("Time taken [CompOld]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

        // Clean up previous approach
        free(op);
        ubpf_destroy(vm);
    }
    // Clean up
    ubpf_unload_code(vm);
    free(input);
    free(ebpf);
    free(err);
    }
    // Experiment 2: Split
    {

    }
    // Experiment 3: Verified vs raw
    {
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
            sizeof(struct layout) + DATA_SIZE +
            DATA_SIZE * sizeof(int) / 8;
        struct layout *input = (struct layout*)malloc(data_sz);
        input->dates_offset = DATA_SIZE;
        input->conv = TO_YEAR;
        FILE *data = fopen("data/dates.dat", "rb");
        fread(input->dynamic, 1, DATA_SIZE, data);
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
        printf("Time taken [Convert]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

        // Clean up
        ubpf_unload_code(vm);
        free(input);
        free(ebpf);
        free(err);
    }
    // Experiment 4: Resource Manager
    {
    // #include "external/delilah/delilah/programs/roger_programs/roger_aggregate_by_pos_shared.c"
    // 1;
    // // Prepare shared regions
    // mem_rng dummy;
    // int element_id = allocate_shared(sizeof(int) * DATA_SIZE, &dummy);
    // memcpy(dummy.ptr, input->dynamic, dummy.length);
    // int idx_id = allocate_shared(sizeof(uint32_t) * DATA_SIZE/2, &dummy);
    // memcpy(dummy.ptr, (input->dynamic)+(input->element_size), dummy.length);
    // request_set dummy_reqs;
    // request reqs[2] = {{element_id, Write}, {idx_id, Write}};
    // dummy_reqs.reqs = reqs;
    // dummy_reqs.count = 2;
    // enqueue_rel(&releases, dummy_reqs);
    // printf("Release successful\n");
    // struct layout input;
    // input.element_req = element_id;
    // input.index_req = idx_id;
    // clock_t start, finish;
    // start = clock();
    // roger_prog(&input);
    // finish = clock();
    // printf("Time taken [Shared] %.3f ms\n", (double)(finish-start)/CLOCKS_PER_SEC*1000);
    }
    // Experiment 5: Composing
    {
        #include "external/delilah/delilah/programs/roger_programs/filter_dates.c"
        ;
        // Load ebpf code
        // FILE *fptr = fopen("programs/filter_dates.o", "rb");
        // fseek(fptr, 0L, SEEK_END);
        // int sz = ftell(fptr);
        // rewind(fptr);
        // char *ebpf = (char*) malloc(sz);
        // fread(ebpf, 1, sz, fptr);
        // char *err = malloc(1024);
        // ubpf_load_elf(vm, ebpf, sz, &err);
        // if(err) printf("errors: %s\n", err);

        // Prepare data
        // struct layout {
        //     int dates_offset;
        //     uint32_t low;
        //     uint32_t high;
        //     int result_start;
        //     int result_end;
        //     char dynamic[];
        // };
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
        printf("Time taken [Composed]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

        // Clean up
        ubpf_unload_code(vm);
        free(input);
        // free(ebpf);
        // free(err);
    }
    destroy_manager(manager);
    ubpf_destroy(vm);
}