#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>

// Voxel volume grid on device: 3D int array with SDF-like initialization
__global__ void init_voxel_grid(int* grid, int nx, int ny, int nz, float voxel_size, float t_snap) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    // World coordinates
    float x = (ix - nx/2) * voxel_size;
    float y = (iy - ny/2) * voxel_size;
    float z = iz * voxel_size;

    // Simple SDF: sphere + box subtraction (voxel carving)
    float r_sphere = 0.02f + 0.005f * t_snap;
    float r_cyl = 0.003f;
    float d_sphere = sqrtf(x*x + y*y + z*z) - r_sphere;
    float d_cyl = sqrtf(x*x + y*y) - r_cyl;

    // Channel carved through sphere
    float d = fmaxf(d_sphere, -d_cyl);

    // Material ID: 0=empty, 1=solid, 2=channel
    int mat = 0;
    if (d < 0.0f) {
        mat = (d_cyl < 0.0f) ? 2 : 1;
    }

    int idx = ix + iy * nx + iz * nx * ny;
    grid[idx] = mat;
}

// Reduction of active voxels per material
__global__ void count_materials(const int* grid, int nx, int ny, int nz,
                                int* out_count0, int* out_count1, int* out_count2) {
    extern __shared__ int sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;

    int c0 = 0, c1 = 0, c2 = 0;
    int total = nx * ny * nz;
    if (i < total) {
        int v = grid[i];
        if (v == 0) c0 = 1;
        else if (v == 1) c1 = 1;
        else c2 = 1;
    }
    sdata[tid] = c0;
    sdata[tid + blockDim.x] = c1;
    sdata[tid + blockDim.x * 2] = c2;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
            sdata[tid + blockDim.x] += sdata[tid + s + blockDim.x];
            sdata[tid + blockDim.x * 2] += sdata[tid + s + blockDim.x * 2];
        }
        __syncthreads();
    }

    if (tid == 0) {
        out_count0[blockIdx.x] = sdata[0];
        out_count1[blockIdx.x] = sdata[blockDim.x];
        out_count2[blockIdx.x] = sdata[blockDim.x * 2];
    }
}

int main() {
    printf("=== CUDA Voxel SDF + Material Count Test ===\n");

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        printf("No CUDA device detected. Skipping runtime test.\n");
        printf("Compilation-only mode: PASS\n");
        return 0;
    }
    printf("Devices: %d\n", deviceCount);

    int nx = 64, ny = 64, nz = 64;
    float voxel_size = 0.0005f;  // 0.5 mm
    float t_snap = 0.5f;

    size_t nvox = (size_t)nx * ny * nz;
    size_t bytes = nvox * sizeof(int);

    int *d_grid = nullptr;
    err = cudaMalloc(&d_grid, bytes);
    if (err != cudaSuccess) {
        printf("cudaMalloc failed: %s\n", cudaGetErrorString(err));
        return 1;
    }

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x,
              (ny + block.y - 1) / block.y,
              (nz + block.z - 1) / block.z);

    init_voxel_grid<<<grid, block>>>(d_grid, nx, ny, nz, voxel_size, t_snap);
    cudaDeviceSynchronize();

    int threads = 256;
    int blocks = (nvox + threads - 1) / threads;
    size_t smem = threads * 3 * sizeof(int);

    int *d_c0 = nullptr, *d_c1 = nullptr, *d_c2 = nullptr;
    cudaMalloc(&d_c0, blocks * sizeof(int));
    cudaMalloc(&d_c1, blocks * sizeof(int));
    cudaMalloc(&d_c2, blocks * sizeof(int));

    count_materials<<<blocks, threads, smem>>>(d_grid, nx, ny, nz, d_c0, d_c1, d_c2);
    cudaDeviceSynchronize();

    int h_c0[1024] = {0}, h_c1[1024] = {0}, h_c2[1024] = {0};
    cudaMemcpy(h_c0, d_c0, blocks * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_c1, d_c1, blocks * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_c2, d_c2, blocks * sizeof(int), cudaMemcpyDeviceToHost);

    int total0 = 0, total1 = 0, total2 = 0;
    for (int i = 0; i < blocks; ++i) {
        total0 += h_c0[i];
        total1 += h_c1[i];
        total2 += h_c2[i];
    }

    printf("Voxel grid %dx%dx%d = %zu voxels\n", nx, ny, nz, nvox);
    printf("Material 0 (empty):   %d\n", total0);
    printf("Material 1 (solid):   %d\n", total1);
    printf("Material 2 (channel): %d\n", total2);
    printf("Total accounted: %d / %zu\n", total0 + total1 + total2, nvox);

    cudaFree(d_grid);
    cudaFree(d_c0);
    cudaFree(d_c1);
    cudaFree(d_c2);

    if (total1 > 0 && total2 > 0) {
        printf("CUDA voxel SDF test passed.\n");
        return 0;
    }
    printf("CUDA voxel SDF test FAILED.\n");
    return 1;
}
