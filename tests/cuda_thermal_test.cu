#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include "nanodb/nanovdb/tools/CudaVoxelTestUtils.h"
#include "nanodb/nanovdb/tools/CudaThermalKernels.h"

using namespace nanovdb::tools::cuda_tools;

int main() {
    printf("=== CUDA Thermal Kernels Exhaustive Test ===\n");

    if (!cuda_tools::verifyDeviceProperties(700)) {
        printf("No suitable CUDA device. Skipping.\n");
        return 0;
    }

    // 1. Device thermal params: UHTC properties
    DeviceThermalParams params;
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
    float *d_T = nullptr, *d_sigma = nullptr, *d_q = nullptr, *d_pen = nullptr, *d_safety = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_T, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sigma, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_q, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_pen, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_safety, bytes), 1);

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x, (ny + block.y - 1) / block.y, (nz + block.z - 1) / block.z);

    // 3. Thermal shock kernel
    thermalShockKernel<<<grid, block>>>(d_T, d_sigma, params, nx, ny, nz, voxel_size, t_snap, z_min, z_max);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 4. Heat flux kernel
    heatFluxKernel<<<grid, block>>>(d_q, nx, ny, nz, 1e6, 3500.0f, 3000.0f, z_min, z_max);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 5. Penetration depth kernel
    float alpha = params.k_w / (params.rho_w * params.cp_w);
    penetrationDepthKernel<<<grid, block>>>(d_pen, nx, ny, nz, voxel_size, alpha, t_snap, z_min, z_max);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 6. Safety factor kernel
    thermalSafetyFactorKernel<<<grid, block>>>(d_safety, nx, ny, nz, d_sigma, d_T, 800e6f, nx*ny);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 7. Thermal stats reduction
    int threads = 256;
    int blocks = (nvox + threads - 1) / threads;
    size_t smem = threads * 6 * sizeof(float);

    float *d_Tmin = nullptr, *d_Tmax = nullptr, *d_Tavg = nullptr;
    float *d_smax = nullptr, *d_savg = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_Tmin, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_Tmax, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_Tavg, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_smax, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_savg, blocks * sizeof(float)), 1);

    thermalStatsReduction<<<blocks, threads, smem>>>(d_T, d_sigma, nvox, d_Tmin, d_Tmax, d_Tavg, d_smax, d_savg);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 8. Copy back stats
    float h_Tmin[1024], h_Tmax[1024], h_Tavg[1024], h_smax[1024], h_savg[1024];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_Tmin, d_Tmin, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_Tmax, d_Tmax, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_Tavg, d_Tavg, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_smax, d_smax, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_savg, d_savg, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);

    float global_Tmin = 1e30f, global_Tmax = -1e30f, global_Tavg = 0.0f;
    float global_smax = -1e30f, global_savg = 0.0f;
    for (int i = 0; i < blocks; ++i) {
        global_Tmin = fminf(global_Tmin, h_Tmin[i]);
        global_Tmax = fmaxf(global_Tmax, h_Tmax[i]);
        global_Tavg += h_Tavg[i];
        global_smax = fmaxf(global_smax, h_smax[i]);
        global_savg += h_savg[i];
    }
    global_Tavg /= nvox;
    global_savg /= nvox;

    printf("Thermal stats:\n");
    printf("  T min/max/avg: %.1f / %.1f / %.1f K\n", global_Tmin, global_Tmax, global_Tavg);
    printf("  |sigma| max/avg: %.1f / %.1f MPa\n", global_smax / 1e6, global_savg / 1e6);

    // 9. Validate physical expectations
    if (global_Tmin < params.T_i || global_Tmax > params.T_aw) {
        printf("FAIL: Temperature out of bounds [%.1f, %.1f]\n", global_Tmin, global_Tmax);
        return 1;
    }

    if (global_smax < 0.0f) {
        printf("FAIL: Max stress negative\n");
        return 1;
    }

    // 10. Copy full fields back for detailed validation
    float *h_T = new float[nvox];
    float *h_sigma = new float[nvox];
    float *h_q = new float[nvox];
    float *h_pen = new float[nvox];
    float *h_safety = new float[nvox];

    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_T, d_T, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sigma, d_sigma, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_q, d_q, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_pen, d_pen, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_safety, d_safety, bytes, cudaMemcpyDeviceToHost), 1);

    // 11. Validate penetration depth mask
    int penetration_count = 0;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_pen[i] > 0.5f) penetration_count++;
    }
    float pen_ratio = (float)penetration_count / nvox;
    printf("Penetration depth ratio: %.2f%%\n", pen_ratio * 100.0f);

    // 12. Validate safety factors
    int unsafe_count = 0;
    float min_safety = 1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_safety[i] < 1.0f) unsafe_count++;
        if (h_safety[i] < min_safety) min_safety = h_safety[i];
    }
    printf("Min safety factor: %.2f, Unsafe voxels: %d / %zu\n", min_safety, unsafe_count, nvox);

    // 13. Time measurement
    CudaTimer timer;
    timer.start();
    thermalShockKernel<<<grid, block>>>(d_T, d_sigma, params, nx, ny, nz, voxel_size, t_snap, z_min, z_max);
    CUDA_CHECK(cudaDeviceSynchronize());
    float thermal_ms = timer.stop();
    printf("Thermal kernel time: %.3f ms\n", thermal_ms);

    // 14. Cleanup
    delete[] h_T; delete[] h_sigma; delete[] h_q; delete[] h_pen; delete[] h_safety;
    cudaFree(d_T); cudaFree(d_sigma); cudaFree(d_q); cudaFree(d_pen); cudaFree(d_safety);
    cudaFree(d_Tmin); cudaFree(d_Tmax); cudaFree(d_Tavg);
    cudaFree(d_smax); cudaFree(d_savg);

    printf("CUDA thermal kernels test passed.\n");
    return 0;
}
