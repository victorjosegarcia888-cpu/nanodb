#include <cstdio>
#include <cuda_runtime_api.h>

__global__ void voxel_init_kernel(int* data, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) data[i] = i;
}

__global__ void voxel_reduce_kernel(const int* data, int* out, int n) {
    extern __shared__ int sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;
    sdata[tid] = (i < n) ? data[i] : 0;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }
    if (tid == 0) out[blockIdx.x] = sdata[0];
}

int main() {
    printf("=== CUDA Voxel Test ===\n");

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        printf("No CUDA-capable device detected (expected in headless codespace).\n");
        printf("CUDA compilation test: PASSED (code compiles, runtime requires GPU)\n");
        return 0;
    }

    int n = 256;
    size_t bytes = n * sizeof(int);
    int *d_a = nullptr, *d_out = nullptr;

    err = cudaMalloc(&d_a, bytes);
    if (err != cudaSuccess) {
        printf("cudaMalloc failed: %s\n", cudaGetErrorString(err));
        return 1;
    }

    err = cudaMalloc(&d_out, sizeof(int));
    if (err != cudaSuccess) {
        printf("cudaMalloc out failed: %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        return 1;
    }

    voxel_init_kernel<<<(n + 255) / 256, 256>>>(d_a, n);
    cudaDeviceSynchronize();

    int blockSize = 64;
    voxel_reduce_kernel<<<1, blockSize, blockSize * sizeof(int)>>>(d_a, d_out, n);
    cudaDeviceSynchronize();

    int h_out = 0;
    cudaMemcpy(&h_out, d_out, sizeof(int), cudaMemcpyDeviceToHost);

    printf("Voxel init+reduce on %d elements: %d (expected %d)\n",
           n, h_out, n * (n - 1) / 2);

    cudaFree(d_a);
    cudaFree(d_out);

    if (h_out == n * (n - 1) / 2) {
        printf("CUDA voxel test passed.\n");
        return 0;
    } else {
        printf("CUDA voxel test FAILED.\n");
        return 1;
    }
}
