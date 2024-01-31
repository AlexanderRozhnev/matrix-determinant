#include "array_memory.h"

double** AllocArrayDouble(size_t N, size_t M) {
    double** array = (double**)malloc(N * sizeof(double*));
    for(int i = 0; i < N; i++) {
        array[i] = (double*)malloc(M * sizeof(double));
    }
    return array;
}

void FreeArray(double** array, size_t N) {
    for(int i = 0; i < N; i++) {
        free(array[i]);
    }
    free(array);
}