#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include "nanodb/nanovdb/tools/CudaVoxelTestUtils.h"
#include "nanodb/nanovdb/tools/CudaRocketKernels.h"

namespace ct = nanovdb::tools::cuda_tools;

int main() {
    printf("=== CUDA Rocket Kernels Exhaustive Test ===\n");

    if (!ct::verifyDeviceProperties(700)) {
        printf("No suitable CUDA device. Skipping.\n");
        return 0;
    }

    // 1. Aerospike parameters
    ct::DeviceAerospikeParams aero_params;
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

    // 2. Allocate device fields
    float *d_sdf = nullptr, *d_composite = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_sdf, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_composite, bytes), 1);

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x, (ny + block.y - 1) / block.y, (nz + block.z - 1) / block.z);

    // 3. Aerospike SDF kernel
    ct::aerospikeSDFKernel<<<grid, block>>>(d_sdf, nx, ny, nz, x_min, y_min, z_min, voxel_size, aero_params);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // 4. Copy back and validate
    float *h_sdf = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_sdf, d_sdf, bytes, cudaMemcpyDeviceToHost), 1);

    float smin = 1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_sdf[i] < smin) smin = h_sdf[i];
    }
    printf("Aerospike SDF min: %.4f\n", smin);

    if (smin > 0.0f) {
        printf("FAIL: No solid voxels found\n");
        return 1;
    }

    // 5. Time measurement
    ct::CudaTimer timer;
    timer.start();
    ct::aerospikeSDFKernel<<<grid, block>>>(d_sdf, nx, ny, nz, x_min, y_min, z_min, voxel_size, aero_params);
    CUDA_CHECK(cudaDeviceSynchronize());
    float aero_ms = timer.stop();
    printf("Aerospike kernel time: %.3f ms\n", aero_ms);

    // 6. Cleanup
    delete[] h_sdf;
    cudaFree(d_sdf); cudaFree(d_composite);

    printf("CUDA rocket kernels test passed.\n");
    return 0;
}
