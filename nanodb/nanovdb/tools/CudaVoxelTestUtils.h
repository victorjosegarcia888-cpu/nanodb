#ifndef NANOVDB_TOOLS_CUDA_VOXEL_TEST_UTILS_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_CUDA_VOXEL_TEST_UTILS_H_HAS_BEEN_INCLUDED

#include <cuda_runtime_api.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <chrono>

namespace nanovdb {
namespace tools {
namespace cuda_tools {

/// @brief RAII wrapper for CUDA error checking
class CudaError {
public:
    CudaError(cudaError_t err, const char* file, int line)
        : m_err(err), m_file(file), m_line(line) {}

    operator bool() const { return m_err != cudaSuccess; }

    void report(const char* msg) const {
        if (m_err != cudaSuccess) {
            fprintf(stderr, "CUDA Error [%s:%d] %s: %s\n",
                    m_file, m_line, msg, cudaGetErrorString(m_err));
        }
    }

private:
    cudaError_t m_err;
    const char* m_file;
    int m_line;
};

#define CUDA_CHECK(call) \
    do { \
        cudaError_t _err = (call); \
        CudaError _ce(_err, __FILE__, __LINE__); \
        if (_ce) _ce.report(#call); \
    } while(0)

#define CUDA_CHECK_AND_RETURN(call, retval) \
    do { \
        cudaError_t _err = (call); \
        CudaError _ce(_err, __FILE__, __LINE__); \
        if (_ce) { _ce.report(#call); return retval; } \
    } while(0)

/// @brief Timing helper for CUDA kernels
class CudaTimer {
public:
    CudaTimer() { CUDA_CHECK(cudaEventCreate(&m_start)); CUDA_CHECK(cudaEventCreate(&m_stop)); }
    ~CudaTimer() { cudaEventDestroy(m_start); cudaEventDestroy(m_stop); }

    void start() { CUDA_CHECK(cudaEventRecord(m_start, 0)); }
    float stop() {
        CUDA_CHECK(cudaEventRecord(m_stop, 0));
        CUDA_CHECK(cudaEventSynchronize(m_stop));
        float ms = 0.0f;
        CUDA_CHECK(cudaEventElapsedTime(&ms, m_start, m_stop));
        return ms;
    }

private:
    cudaEvent_t m_start, m_stop;
};

/// @brief Host-side validation helpers for voxel grids
inline bool validateMaterialCounts(const int* h_grid, size_t nvox,
                                   int expected_empty, int expected_solid, int expected_channel,
                                   int tolerance_percent = 1) {
    int count0 = 0, count1 = 0, count2 = 0;
    for (size_t i = 0; i < nvox; ++i) {
        int v = h_grid[i];
        if (v == 0) count0++;
        else if (v == 1) count1++;
        else if (v == 2) count2++;
        else {
            fprintf(stderr, "Invalid material ID %d at voxel %zu\n", v, i);
            return false;
        }
    }

    int total = count0 + count1 + count2;
    if (total != (int)nvox) {
        fprintf(stderr, "Material count mismatch: %d / %zu\n", total, nvox);
        return false;
    }

    int tol = (int)nvox * tolerance_percent / 100;
    if (abs(count0 - expected_empty) > tol ||
        abs(count1 - expected_solid) > tol ||
        abs(count2 - expected_channel) > tol) {
        fprintf(stderr, "Material distribution out of tolerance: %d/%d/%d vs expected %d/%d/%d\n",
                count0, count1, count2, expected_empty, expected_solid, expected_channel);
        return false;
    }

    return true;
}

/// @brief Validate that a SDF sphere is correctly carved
inline bool validateSphereCarving(const int* h_grid, int nx, int ny, int nz,
                                  float voxel_size, float expected_radius, float tolerance = 0.1f) {
    int cx = nx / 2, cy = ny / 2, cz = nz / 2;
    int r_vox = static_cast<int>(expected_radius / voxel_size) + 1;

    for (int iz = cz - r_vox; iz <= cz + r_vox; ++iz) {
        for (int iy = cy - r_vox; iy <= cy + r_vox; ++iy) {
            for (int ix = cx - r_vox; ix <= cx + r_vox; ++ix) {
                if (ix < 0 || ix >= nx || iy < 0 || iy >= ny || iz < 0 || iz >= nz) continue;

                float x = (ix - cx) * voxel_size;
                float y = (iy - cy) * voxel_size;
                float z = (iz - cz) * voxel_size;
                float d = sqrtf(x*x + y*y + z*z) - expected_radius;

                int idx = ix + iy * nx + iz * nx * ny;
                int mat = h_grid[idx];

                if (d < -voxel_size * 0.5f && mat == 0) {
                    fprintf(stderr, "Solid voxel marked empty at (%d,%d,%d), d=%.4f\n", ix, iy, iz, d);
                    return false;
                }
                if (d > voxel_size * 0.5f && mat != 0) {
                    fprintf(stderr, "Empty voxel marked solid at (%d,%d,%d), d=%.4f\n", ix, iy, iz, d);
                    return false;
                }
            }
        }
    }
    return true;
}

/// @brief Validate that a cylindrical channel exists along Z axis
inline bool validateChannelCarving(const int* h_grid, int nx, int ny, int nz,
                                   float voxel_size, float expected_radius) {
    int cx = nx / 2, cy = ny / 2;
    int r_vox = static_cast<int>(expected_radius / voxel_size) + 1;

    for (int iz = 0; iz < nz; ++iz) {
        for (int iy = cy - r_vox; iy <= cy + r_vox; ++iy) {
            for (int ix = cx - r_vox; ix <= cx + r_vox; ++ix) {
                if (ix < 0 || ix >= nx || iy < 0 || iy >= ny) continue;

                float x = (ix - cx) * voxel_size;
                float y = (iy - cy) * voxel_size;
                float d_cyl = sqrtf(x*x + y*y) - expected_radius;

                int idx = ix + iy * nx + iz * nx * ny;
                int mat = h_grid[idx];

                if (d_cyl < -voxel_size * 0.5f && mat != 2) {
                    fprintf(stderr, "Channel voxel not material 2 at (%d,%d,%d), d_cyl=%.4f, mat=%d\n",
                            ix, iy, iz, d_cyl, mat);
                    return false;
                }
            }
        }
    }
    return true;
}

/// @brief Measure and report kernel execution time
inline float measureKernelTime(void (*kernel)(), int iterations = 10) {
    CudaTimer timer;
    float total_ms = 0.0f;

    for (int i = 0; i < iterations; ++i) {
        timer.start();
        kernel();
        CUDA_CHECK(cudaDeviceSynchronize());
        total_ms += timer.stop();
    }

    float avg_ms = total_ms / iterations;
    printf("Kernel avg time: %.3f ms (%d iterations)\n", avg_ms, iterations);
    return avg_ms;
}

/// @brief Verify CUDA device properties
inline bool verifyDeviceProperties(int minComputeCapability = 700) {
    int device = 0;
    cudaDeviceProp prop;
    CUDA_CHECK_AND_RETURN(cudaGetDeviceProperties(&prop, device), false);

    printf("Device: %s\n", prop.name);
    printf("Compute capability: %d.%d\n", prop.major, prop.minor);
    printf("Total global memory: %.2f GB\n", prop.totalGlobalMem / (1024.0*1024.0*1024.0));
    printf("Multiprocessors: %d\n", prop.multiProcessorCount);
    printf("Max threads per block: %d\n", prop.maxThreadsPerBlock);

    int cc = prop.major * 100 + prop.minor * 10;
    if (cc < minComputeCapability) {
        fprintf(stderr, "Device compute capability %d.%d below minimum %d.%d\n",
                prop.major, prop.minor, minComputeCapability/100, (minComputeCapability%100)/10);
        return false;
    }
    return true;
}

/// @brief Allocate and zero-initialize device memory with size check
inline int* allocateDeviceVoxelGrid(size_t nvox, size_t* out_bytes) {
    *out_bytes = nvox * sizeof(int);
    int* d_grid = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_grid, *out_bytes), nullptr);
    CUDA_CHECK_AND_RETURN(cudaMemset(d_grid, 0, *out_bytes), nullptr);
    return d_grid;
}

} // namespace cuda_tools
} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_CUDA_VOXEL_TEST_UTILS_H_HAS_BEEN_INCLUDED
