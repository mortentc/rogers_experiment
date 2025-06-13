#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ubpf.h"
#include "verified_functions.h"
#include <time.h>
#include <delilah.h>
#include "external/delilah/delilah/programs/roger_programs/roger_aggregate_by_pos.c"
#include "external/delilah/delilah/programs/aggregate_by_pos.c"

#define DATA_SIZE 100000000
#define RUNS 10

void dummy(void){return;}
mem_rng *obtain(struct request *reqs, int num){
    mem_rng *dummy = malloc(sizeof(mem_rng));
    dummy->ptr = (char*)malloc(12);
    dummy->length = 12;
    return dummy;
};

int main(){
    // Setup Ubpf VM
    struct ubpf_vm* vm = ubpf_create();
    ubpf_register(vm, 2, "obtain", &obtain);
    int res = ubpf_register(vm, 1, "aggregate_by_pos", &aggregate_by_pos);
    printf("Registering code: %s\n", res ? "failed" : "succeeded");

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
    FILE *data = fopen("data/elements.dat", "rb");
    fread(input->dynamic, 4, DATA_SIZE*3/2, data);
    fclose(data);

    FILE *log = fopen("data/results.txt", "w");
    char out[10];
    // Execute program
    uint64_t result;
    clock_t start, finish;
    clock_t aggr_time = 0;
    for(int i = 0; i<RUNS; i++){
        start = clock();
        ubpf_exec(vm, input, data_sz, &result, NULL, 0);
        finish = clock();
        sprintf(out, "%.3f\n", ((double)(finish-start))/CLOCKS_PER_SEC*1000);
        fwrite(out, strlen(out), 1, log);
        aggr_time += finish - start;
    }
    
    // sprintf(out, "%.3f\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
    // fwrite(out, strlen(out), 1, log);
    // float start = (float)clock();
    // ubpf_exec(vm, input, data_sz, &result, NULL, 0);
    // float finish = (float)clock();
    //printf("Execution result: %d\n", input->result);
    // printf("Length of shared region: %ld\n", result);
    printf("Time taken [MixedVerif]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

    // start = (float)clock();
    // roger_prog(input);
    // finish = (float)clock();
    aggr_time = 0;
    for(int i = 0; i<RUNS; i++){
        start = clock();
        roger_prog(input);
        finish = clock();
        sprintf(out, "%.3f\n", ((double)(finish-start))/CLOCKS_PER_SEC*1000);
        fwrite(out, strlen(out), 1, log);
        aggr_time += finish - start;
    }
    // sprintf(out, "%.3f\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
    // fwrite(out, strlen(out), 1, log);
    //printf("Execution result: %d\n", input->result);
    printf("Time taken [CompVerif]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);

    // Previous approach
    {
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
        void *shared = positions+DATA_SIZE/2;//malloc(DATA_SIZE*sizeof(int));
        memcpy(shared, input->dynamic, DATA_SIZE*sizeof(int));

        
        // void *shared = malloc(1024);
        // clock_t start = clock();
        // ubpf_exec(vm, op, data_sz, &result, shared, DATA_SIZE*sizeof(int));
        // clock_t finish = clock();
        // printf("==Old execution==\n");
        //printf("Execution result: %d\n", *(uint32_t*)((char*)op+op->result_offset));
        // printf("Length of shared region: %ld\n", result);
        // printf("Time taken [VmOld]: %.3f ms\n", ((float)(finish-start))/CLOCKS_PER_SEC*1000);
        aggr_time = 0;
        for(int i = 0; i<RUNS; i++){
            start = clock();
            ubpf_exec(vm, op, data_sz, &result, shared, DATA_SIZE*sizeof(int));
            finish = clock();
            sprintf(out, "%.3f\n", ((double)(finish-start))/CLOCKS_PER_SEC*1000);
            fwrite(out, strlen(out), 1, log);
            aggr_time += finish - start;
        }
        // sprintf(out, "%.3f\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
        // fwrite(out, strlen(out), 1, log);
        printf("Time taken [VmOld]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
        aggr_time = 0;
        for(int i = 0; i<RUNS; i++){
            start = clock();
            prog(op, data_sz, shared, DATA_SIZE*sizeof(int));
            finish = clock();
            sprintf(out, "%.3f\n", ((double)(finish-start))/CLOCKS_PER_SEC*1000);
            fwrite(out, strlen(out), 1, log);
            aggr_time += finish - start;
        }
        // sprintf(out, "%.3f\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
        // fwrite(out, strlen(out), 1, log);
        printf("Time taken [CompOld]: %.3f ms\n", ((double)aggr_time)/CLOCKS_PER_SEC*1000/RUNS);
        // start = clock();
        // prog(op, data_sz, shared, DATA_SIZE*sizeof(int));
        // finish = clock();
        //printf("Execution result: %d\n", *(uint32_t*)((char*)op+op->result_offset));
        // printf("Length of shared region: %ld\n", result);
        // printf("Time taken [CompOld]: %.3f ms\n", ((float)(finish-start))/CLOCKS_PER_SEC*1000);

        // free(shared);
        free(op);
        ubpf_destroy(vm);
    }

    // Clean up
    free(input);
    free(ebpf);
    free(err);
    ubpf_destroy(vm);
}