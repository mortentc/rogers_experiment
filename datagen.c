#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define DATA_SIZE 1000000

int data[DATA_SIZE];
uint32_t indices[DATA_SIZE / 2];

int main(){
    for(int i = 0; i<DATA_SIZE; i++) data[i] = rand() % 20000;
    for(int i = 0; i<DATA_SIZE/2; i++) indices[i] = rand() % DATA_SIZE;
    FILE *f = fopen("data/elements.dat", "wb");
    fwrite(data, sizeof(int), DATA_SIZE, f);
    fwrite(indices, sizeof(uint32_t), DATA_SIZE/2, f);
    long int res = 0; for(int i = 0; i<DATA_SIZE/2;i++) res += data[indices[i]];
    printf("Expected result %ld\n", res);
    fclose(f);
}