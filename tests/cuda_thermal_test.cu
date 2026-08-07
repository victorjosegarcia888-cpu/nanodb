#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include "nanodb/nanovdb/tools/CudaVoxelTestUtils.h"
#include "nanodb/nanovdb/tools/CudaThermalKernels.h"

namespace ct = nanovdb::tools::cuda_tools;

int main() {
    printf("=== CUDA Thermal Kernels Exhaustive Test ===\n");

    if (!ct::verifyDeviceProperties(700)) {
        printf("No suitable CUDA device. Skipping.\n");
        return 0;
    }

    // 1. Device thermal params: UHTC properties
    ct::DeviceThermalParams params;
    params.k_w = 80.0;
    params.rho_w = 6000.0;
    params.cp_w = 500.0;
    params.alpha_te = 5.5e-6;
    params.E = 380e9;
    params.nu = 0.18;
    params.T_i = 300.0;
    params.T_aw = 3500.0;
    params.h_g = 1e6;

    int nx = 32, ny = 32, nz = 64;
    float voxel_size = 0.0005f;
    float t_snap = 0.5f;
    float z_min = 0.0f, z_max = 0.032f;
    size_t nvox = (size_t)nx * ny * nz;
    size_t bytes = nvox * sizeof(float);

    // 2. Allocate device fields
    float *d_T = nullptr, *d_sigma = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_T, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sigma, bytes), 1);

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x, (ny + block.y - 1) / block.y, (nz + block.z - 1) / block.z);

    // 3. Thermal shock kernel
    ct::thermalShockKernel<<<grid, block>>>(d_T, d_sigma, params, nx, ny, nz, voxel_size, t_snap, z_min, z_max);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 4. Copy full fields back for detailed validation
    float *h_T = new float[nvox];
    float *h_sigma = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_T, d_T, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sigma, d_sigma, bytes, cudaMemcpyDeviceToHost), 1);

    float Tmin = 1e30f, Tmax = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_T[i] < Tmin) Tmin = h_T[i];
        if (h_T[i] > Tmax) Tmax = h_T[i];
    }
    printf("T min/max: %.1f / %.1f K\n", Tmin, Tmax);

    if (Tmin < params.T_i || Tmax > params.T_aw) {
        printf("FAIL: Temperature out of bounds [%.1f, %.1f]\n", Tmin, Tmax);
        return 1;
    }

    // 5. Time measurement
    ct::CudaTimer timer;
    timer.start();
    ct::thermalShockKernel<<<grid, block>>>(d_T, d_sigma, params, nx, ny, nz, voxel_size, t_snap, z_min, z_max);
    CUDA_CHECK(cudaDeviceSynchronize());
    float thermal_ms = timer.stop();
    printf("Thermal kernel time: %.3f ms\n", thermal_ms);

    // 6. Cleanup
    delete[] h_T; delete[] h_sigma;
    cudaFree(d_T); cudaFree(d_sigma);

    printf("CUDA thermal kernels test passed.\n");
    return 0;
}
