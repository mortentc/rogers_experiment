#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define DATA_SIZE 1000000


int main(){
    // Generate aggregation data
    int data[DATA_SIZE];
    uint32_t indices[DATA_SIZE / 2];
    for(int i = 0; i<DATA_SIZE; i++) data[i] = rand() % 20000;
    for(int i = 0; i<DATA_SIZE/2; i++) indices[i] = rand() % DATA_SIZE;
    FILE *f = fopen("data/sum.dat", "wb");
    fwrite(data, sizeof(int), DATA_SIZE, f);
    fwrite(indices, sizeof(uint32_t), DATA_SIZE/2, f);
    fclose(f);
    // long int res = 0; for(int i = 0; i<DATA_SIZE/2;i++) res += data[indices[i]];
    // can easily overflow, but values should still agree
    // printf("Expected result %ld\n", res);

    // Generate ASCII digits
    char dates[DATA_SIZE*4];
    for(int i = 0; i<DATA_SIZE*4; i++) dates[i] = rand() % 10 + '0';
    FILE *g = fopen("data/dates.dat", "wb");
    fwrite(dates, 1, DATA_SIZE*4, f);
    fclose(g);
}