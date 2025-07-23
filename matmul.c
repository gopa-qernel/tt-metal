#include <stdio.h>
#include <stdlib.h>

#define N 512

void matmul(float* A, float* B, float* C, int n) {
    #pragma omp parallel for
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < n; ++k)
                sum += A[i*n + k] * B[k*n + j];
            C[i*n + j] = sum;
        }
}

int main() {
    float* A = (float*)malloc(N*N*sizeof(float));
    float* B = (float*)malloc(N*N*sizeof(float));
    float* C = (float*)malloc(N*N*sizeof(float));
    // Initialize A and B
    for (int i = 0; i < N*N; ++i) {
        A[i] = 1.0f;
        B[i] = 1.0f;
    }
    matmul(A, B, C, N);
    printf("C[0]=%f\n", C[0]);
    free(A); free(B); free(C);
    return 0;
}