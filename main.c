#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "external/delilah/delilah/programs/rogers_programs/rogers_aggregate_by_pos.c"
#include "external/delilah/delilah/programs/aggregate_by_pos.c"
// #include "external/delilah/delilah/programs/convert_date.c"
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
uint64_t delilah_file_read_offset(char *a, uint64_t b, uint64_t c, char* d){ return DATA_SIZE; }

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
    printf("Time taken [MixedVerif]: %.3f ms\n", avg_time);

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
        for(int i = 0; i<10; i++){
            start = clock();
            ubpf_exec(vm, op, data_sz, &result, shared, DATA_SIZE*sizeof(int));
            finish = clock();
            aggr_time += finish - start;
        }
        avg_time = ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS;
        sprintf(out, "%.3f\n", avg_time);
        fwrite(out, strlen(out), 1, log);
        if(PRINT_TO_TERMINAL)
        printf("Time taken [VmOld]: %.3f ms\n", avg_time);

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
        printf("Time taken [CompOld]: %.3f ms\n", avg_time);

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
        #include "external/delilah/delilah/programs/convert_date.c"
        
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
        printf("Time taken [TO_YEAR]: %.3f ms\n", avg_time);

        conversion_op *original_input = (conversion_op*)malloc(sizeof(conversion_op)+DATA_SIZE);
        memcpy(original_input+1, input->dynamic, DATA_SIZE);
        original_input->use_cache = false;
        original_input->reuse_data = true;
        original_input->padding_offset = sizeof(conversion_op);
        original_input->file.size = DATA_SIZE;
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
        printf("Time taken [TO_YEAR][OLD]: %.3f ms\n", avg_time);

        // Clean up
        ubpf_unload_code(vm);
        free(input);
        free(ebpf);
        free(err);
        free(original_input);
    }
    // Experiment 4: Resource Manager
    {
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
    clock_t aggr_time = 0;
    double avg_time = 0;
    
    // Execute empty CSF
    // Warmup
    for(int i = 0; i<WARMUP; i++) ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        aggr_time += finish - start;
    }
    avg_time = ((double)aggr_time)/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
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
    avg_time = ((double)aggr_time)/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Obtain1]: %.3f µs\n", avg_time);

    avg_time = ((double)release_time)/RUNS;
    sprintf(out, "%.3f\n", avg_time);
    fwrite(out, strlen(out), 1, log);
    if(PRINT_TO_TERMINAL)
    printf("Time taken [Release1]: %.3f µs\n", avg_time);

    // Clean up
    ubpf_unload_code(vm);
    free(input);
    free(ebpf);
    free(err);

    // // Prepare shared regions
    // struct layout {
    //     int element_req;
    //     int index_req;
    //     int result;
    //     char dynamic[];
    // } input;
    // mem_rng dummy;
    // int element_id = allocate_shared(sizeof(int) * DATA_SIZE, &dummy);
    // memcpy(dummy.ptr, input.dynamic, dummy.length);
    // int idx_id = allocate_shared(sizeof(uint32_t) * DATA_SIZE/2, &dummy);
    // memcpy(dummy.ptr, (input.dynamic)+(input.element_size), dummy.length);
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
        #include "external/delilah/delilah/programs/rogers_programs/filter_dates.c"
        
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
        printf("Time taken [Composed]: %.3f ms\n", avg_time);

        // #include "external/delilah/delilah/programs/filter_convert_date.c"
        uint32_t context_size = sizeof(fused_filter_conversion_op) + DATA_SIZE;
        fused_filter_conversion_op *original = (fused_filter_conversion_op*)malloc(
            context_size + DATA_SIZE/2 + sizeof(struct delilah_io_state_t)
        );
        original->padding_offset = context_size;
        original->cached_data_offset = sizeof(fused_filter_conversion_op);
        original->file.size = DATA_SIZE;
        original->conv_type = TO_YEAR;
        original->comp_type = BWI;
        original->comp0 = 1000;
        original->comp1 = 2000;
        memcpy(original+1, input->dynamic, DATA_SIZE);

        aggr_time = 0;
        for(int i = 0; i<WARMUP; i++) prog(original, 0, original, 0);
        for(int i = 0; i<RUNS; i++){
            start = clock();
            prog(original, 0, original, 0);
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
        free(original);
        // free(ebpf);
        // free(err);
    }
    destroy_manager(manager);
    ubpf_destroy(vm);
}