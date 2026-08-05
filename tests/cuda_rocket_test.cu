#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include "nanodb/nanovdb/tools/CudaVoxelTestUtils.h"
#include "nanodb/nanovdb/tools/CudaRocketKernels.h"

using namespace nanovdb::tools::cuda_tools;

int main() {
    printf("=== CUDA Rocket Kernels Exhaustive Test ===\n");

    if (!cuda_tools::verifyDeviceProperties(700)) {
        printf("No suitable CUDA device. Skipping.\n");
        return 0;
    }

    // 1. Aerospike parameters
    DeviceAerospikeParams aero_params;
    aero_params.throat_radius = 0.05;
    aero_params.nu_exit = 1.5;
    aero_params.spike_length = 0.5;
    aero_params.channel_width = 0.01;
    aero_params.gyroid_scale = 0.02;
    aero_params.gamma = 1.4;

    int nx = 64, ny = 64, nz = 128;
    float x_min = -0.1f, y_min = -0.1f, z_min = 0.0f;
    float voxel_size = 0.002f;
    size_t nvox = (size_t)nx * ny * nz;
    size_t bytes = nvox * sizeof(float);
    size_t bytes_int = nvox * sizeof(int);

    // 2. Allocate device fields
    float *d_sdf = nullptr, *d_channel = nullptr, *d_composite = nullptr;
    int *d_mat = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sdf, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_channel, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_composite, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_mat, bytes_int), 1);

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x, (ny + block.y - 1) / block.y, (nz + block.z - 1) / block.z);

    // 3. Aerospike SDF kernel
    aerospikeSDFKernel<<<grid, block>>>(d_sdf, nx, ny, nz, x_min, y_min, z_min, voxel_size, aero_params);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 4. Channel SDF kernel
    DeviceSupercriticalChannelParams chan_params;
    chan_params.P = 101325.0;
    chan_params.T_bulk = 300.0;
    chan_params.G = 100.0;
    chan_params.q_flux = 1e6;
    chan_params.D_h = 0.01;
    chan_params.T_pseudocritical = 33.2;
    chan_params.Delta_T_critical = 5.0;
    chan_params.HTD_Suppression_Factor = 0.3f;
    chan_params.base_width = 0.005f;

    supercriticalChannelKernel<<<grid, block>>>(d_channel, nx, ny, nz, x_min, y_min, z_min,
                                                voxel_size, chan_params);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 5. Composite solid kernel
    compositeSolidKernel<<<grid, block>>>(d_composite, nx, ny, nz, d_sdf, d_channel);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 6. Material assignment kernel
    aerospikeMaterialKernel<<<grid, block>>>(d_mat, d_composite, nx, ny, nz, -0.005f);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 7. Schmucker separation ratio
    int schmucker_nvox = 1;
    double *d_p_ratio = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_p_ratio, sizeof(double)), 1);
    schmuckerKernel<<<1, 1>>>(d_p_ratio, 1, 2.0, 1.4);
    CUDA_CHECK(cudaDeviceSynchronize());

    double h_p_ratio = 0.0;
    CUDA_CHECK_AND_RETURN(cudaMemcpy(&h_p_ratio, d_p_ratio, sizeof(double), cudaMemcpyDeviceToHost), 1);

    // 8. Reduction for rocket stats
    int threads = 256;
    int blocks = (nvox + threads - 1) / threads;
    size_t smem = threads * 5 * sizeof(float) + threads * 2 * sizeof(int);

    float *d_sdf_min = nullptr, *d_sdf_max = nullptr, *d_sdf_avg = nullptr;
    int *d_solid_count = nullptr, *d_channel_count = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sdf_min, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sdf_max, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sdf_avg, blocks * sizeof(float)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_solid_count, blocks * sizeof(int)), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_channel_count, blocks * sizeof(int)), 1);

    rocketStatsReduction<<<blocks, threads, smem>>>(d_composite, nvox, d_sdf_min, d_sdf_max,
                                                    d_sdf_avg, d_solid_count, d_channel_count);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 9. Copy back stats
    float h_sdf_min[1024], h_sdf_max[1024], h_sdf_avg[1024];
    int h_solid[1024], h_chan[1024];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sdf_min, d_sdf_min, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sdf_max, d_sdf_max, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sdf_avg, d_sdf_avg, blocks * sizeof(float), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_solid, d_solid_count, blocks * sizeof(int), cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_chan, d_channel_count, blocks * sizeof(int), cudaMemcpyDeviceToHost), 1);

    float global_sdf_min = 1e30f, global_sdf_max = -1e30f, global_sdf_avg = 0.0f;
    int total_solid = 0, total_chan = 0;
    for (int i = 0; i < blocks; ++i) {
        global_sdf_min = fminf(global_sdf_min, h_sdf_min[i]);
        global_sdf_max = fmaxf(global_sdf_max, h_sdf_max[i]);
        global_sdf_avg += h_sdf_avg[i];
        total_solid += h_solid[i];
        total_chan += h_chan[i];
    }
    global_sdf_avg /= nvox;

    printf("Aerospike SDF stats:\n");
    printf("  SDF min/max/avg: %.4f / %.4f / %.4f\n", global_sdf_min, global_sdf_max, global_sdf_avg);
    printf("  Solid voxels: %d\n", total_solid);
    printf("  Channel voxels: %d\n", total_chan);

    // 10. Validate composite SDF bounds
    if (global_sdf_min > 0.0f) {
        printf("FAIL: No solid voxels found (all SDF > 0)\n");
        return 1;
    }

    // 11. Validate Schmucker ratio
    double expected_ratio = pow(1.88 * (2.0 * 2.0 - 1.0) + 1.0, -0.64);
    if (fabs(h_p_ratio - expected_ratio) > 1e-6) {
        printf("FAIL: Schmucker ratio mismatch: %.6f vs %.6f\n", h_p_ratio, expected_ratio);
        return 1;
    }
    printf("Schmucker separation ratio: %.4f\n", h_p_ratio);

    // 12. Copy full fields back for validation
    float *h_sdf = new float[nvox];
    float *h_composite = new float[nvox];
    int *h_mat = new int[nvox];

    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sdf, d_sdf, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_composite, d_composite, bytes, cudaMemcpyDeviceToHost), 1);
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_mat, d_mat, bytes_int, cudaMemcpyDeviceToHost), 1);

    // 13. Validate material IDs are 0, 1, 2 only
    for (size_t i = 0; i < nvox; ++i) {
        int m = h_mat[i];
        if (m != 0 && m != 1 && m != 2) {
            printf("FAIL: Invalid material ID %d at voxel %zu\n", m, i);
            delete[] h_sdf; delete[] h_composite; delete[] h_mat;
            return 1;
        }
    }

    // 14. Validate aerospike at Z=0 (base should be solid)
    int iz_base = 0;
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix < nx; ++ix) {
            int idx = ix + iy * nx + iz_base * nx * ny;
            if (h_composite[idx] > 0.0f) {
                printf("FAIL: Aerospike base has empty voxel at (%d,%d,0), sdf=%.4f\n", ix, iy, h_composite[idx]);
                delete[] h_sdf; delete[] h_composite; delete[] h_mat;
                return 1;
            }
        }
    }

    // 15. Time measurement
    CudaTimer timer;
    timer.start();
    aerospikeSDFKernel<<<grid, block>>>(d_sdf, nx, ny, nz, x_min, y_min, z_min, voxel_size, aero_params);
    CUDA_CHECK(cudaDeviceSynchronize());
    float aero_ms = timer.stop();
    printf("Aerospike kernel time: %.3f ms\n", aero_ms);

    timer.start();
    compositeSolidKernel<<<grid, block>>>(d_composite, nx, ny, nz, d_sdf, d_channel);
    CUDA_CHECK(cudaDeviceSynchronize());
    float composite_ms = timer.stop();
    printf("Composite kernel time: %.3f ms\n", composite_ms);

    // 16. Cleanup
    delete[] h_sdf; delete[] h_composite; delete[] h_mat;
    cudaFree(d_sdf); cudaFree(d_channel); cudaFree(d_composite); cudaFree(d_mat);
    cudaFree(d_p_ratio);
    cudaFree(d_sdf_min); cudaFree(d_sdf_max); cudaFree(d_sdf_avg);
    cudaFree(d_solid_count); cudaFree(d_channel_count);

    printf("CUDA rocket kernels test passed.\n");
    return 0;
}
