#ifndef NANOVDB_TOOLS_CUDA_ROCKET_KERNELS_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_CUDA_ROCKET_KERNELS_H_HAS_BEEN_INCLUDED

#include <cuda_runtime_api.h>
#include <cstdio>
#include <cmath>

namespace nanovdb {
namespace tools {
namespace cuda_tools {

/// @brief Aerospike plug parameters
struct DeviceAerospikeParams {
    double throat_radius;
    double nu_exit;
    double spike_length;
    double channel_width;
    double gyroid_scale;
    double gamma;
};

/// @brief Compute Prandtl-Meyer expansion angle on device
__device__ double prandtlMeyerNu(double M, double gamma = 1.4) {
    if (M <= 1.0) return 0.0;
    double ratio = (gamma - 1.0) / (gamma + 1.0);
    double term1 = sqrt((gamma + 1.0) / (gamma - 1.0));
    double term2 = atan(sqrt(ratio * (M * M - 1.0)));
    double term3 = atan(M * M - 1.0);
    return term1 * term2 - term3;
}

/// @brief Aerospike plug body SDF (positive = solid)
__device__ float aerospikePlugBody(float x, float y, float z,
                                   const DeviceAerospikeParams params) {
    if (z < 0.0f || z > (float)params.spike_length) return 1e6f;

    double nu_exit = prandtlMeyerNu(params.nu_exit, params.gamma);
    double k = tan(nu_exit) * 0.1;
    double r_spike = params.throat_radius - k * z;
    if (r_spike < 0.05 * params.throat_radius) r_spike = 0.05 * params.throat_radius;

    float r = sqrtf(x*x + y*y);
    return r - (float)r_spike;
}

/// @brief Gyroid cooling channel SDF (negative = channel)
__device__ float gyroidChannels(float x, float y, float z,
                                const DeviceAerospikeParams params) {
    double gx = sin(2.0 * M_PI * x / params.gyroid_scale)
              + sin(2.0 * M_PI * y / params.gyroid_scale)
              + sin(2.0 * M_PI * z / params.gyroid_scale);
    double half_width = params.channel_width * 0.5;
    return (float)(gx - half_width);
}

/// @brief Composite solid SDF: plug minus gyroid channels
__device__ float aerospikeCompositeSDF(float x, float y, float z,
                                       const DeviceAerospikeParams params) {
    float f_spike = aerospikePlugBody(x, y, z, params);
    float f_channels = gyroidChannels(x, y, z, params);
    return fmaxf(f_spike, -f_channels);
}

/// @brief CUDA kernel: compute aerospike composite SDF field
__global__ void aerospikeSDFKernel(float* sdf_field, int nx, int ny, int nz,
                                   float x_min, float y_min, float z_min,
                                   float voxel_size, DeviceAerospikeParams params) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    float x = x_min + ix * voxel_size;
    float y = y_min + iy * voxel_size;
    float z = z_min + iz * voxel_size;

    float sdf = aerospikeCompositeSDF(x, y, z, params);

    int idx = ix + iy * nx + iz * nx * ny;
    sdf_field[idx] = sdf;
}

/// @brief CUDA kernel: material ID from composite SDF
__global__ void aerospikeMaterialKernel(int* mat_field, float* sdf_field, int nx, int ny, int nz,
                                        float channel_threshold) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float sdf = sdf_field[idx];

    int mat = 0; // empty
    if (sdf < 0.0f) {
        mat = (sdf < channel_threshold) ? 2 : 1; // 2=channel, 1=solid
    }

    mat_field[idx] = mat;
}

/// @brief Schmucker separation ratio kernel
__device__ double schmuckerSeparationRatio(double M_sep, double gamma = 1.4) {
    return pow(1.88 * (M_sep * M_sep - 1.0) + 1.0, -0.64);
}

/// @brief CUDA kernel: compute Schmucker ratio per voxel (for oblique shock analysis)
__global__ void schmuckerKernel(double* p_ratio, int nvox, double M_sep, double gamma) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= nvox) return;

    p_ratio[i] = schmuckerSeparationRatio(M_sep, gamma);
}

/// @brief Supercritical channel parameters
struct DeviceSupercriticalChannelParams {
    double P;
    double T_bulk;
    double G;
    double q_flux;
    double D_h;
    double T_pseudocritical;
    double Delta_T_critical;
    float HTD_Suppression_Factor;
    float base_width;
};

/// @brief Compute channel width with HTD suppression
__device__ float computeChannelWidth(float x, float y, float z,
                                     const DeviceSupercriticalChannelParams params) {
    float T_bulk = (float)params.T_bulk;
    float T_pc = (float)params.T_pseudocritical;
    float Delta_Tc = (float)params.Delta_T_critical;

    float width = params.base_width;
    if (fabsf(T_bulk - T_pc) < Delta_Tc) {
        width *= (1.0f - params.HTD_Suppression_Factor);
    }
    return width;
}

/// @brief CUDA kernel: supercritical channel SDF field
__global__ void supercriticalChannelKernel(float* channel_sdf, int nx, int ny, int nz,
                                           float x_min, float y_min, float z_min,
                                           float voxel_size,
                                           DeviceSupercriticalChannelParams params) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    float x = x_min + ix * voxel_size;
    float y = y_min + iy * voxel_size;
    float z = z_min + iz * voxel_size;

    float width = computeChannelWidth(x, y, z, params);
    float r = sqrtf(x*x + y*y);
    float sdf = r - width * 0.5f;

    int idx = ix + iy * nx + iz * nx * ny;
    channel_sdf[idx] = sdf;
}

/// @brief CUDA kernel: composite solid with aerospike + channels
__global__ void compositeSolidKernel(float* composite_sdf, int nx, int ny, int nz,
                                     const float* aerospike_sdf, const float* channel_sdf) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float f_spike = aerospike_sdf[idx];
    float f_chan = channel_sdf[idx];
    composite_sdf[idx] = fmaxf(f_spike, -f_chan);
}

/// @brief Reduction kernel for rocket geometry statistics
__global__ void rocketStatsReduction(const float* sdf_field, int nvox,
                                     float* out_min_sdf, float* out_max_sdf,
                                     float* out_avg_sdf, int* out_solid_count,
                                     int* out_channel_count) {
    extern __shared__ float sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;

    float local_min = 1e30f, local_max = -1e30f, local_sum = 0.0f;
    int local_solid = 0, local_channel = 0;

    if (i < nvox) {
        float sdf = sdf_field[i];
        local_min = sdf; local_max = sdf; local_sum = sdf;
        if (sdf < 0.0f) local_solid = 1;
        else if (sdf < -0.001f) local_channel = 1;
    }

    sdata[tid] = local_min;
    sdata[tid + blockDim.x] = local_max;
    sdata[tid + blockDim.x * 2] = local_sum;
    ((int*)&sdata[blockDim.x * 3])[tid] = local_solid;
    ((int*)&sdata[blockDim.x * 3 + blockDim.x])[tid] = local_channel;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] = fminf(sdata[tid], sdata[tid + s]);
            sdata[tid + blockDim.x] = fmaxf(sdata[tid + blockDim.x], sdata[tid + blockDim.x + s]);
            sdata[tid + blockDim.x * 2] += sdata[tid + blockDim.x * 2 + s];
            ((int*)&sdata[blockDim.x * 3])[tid] += ((int*)&sdata[blockDim.x * 3 + s])[tid];
            ((int*)&sdata[blockDim.x * 3 + blockDim.x])[tid] += ((int*)&sdata[blockDim.x * 3 + blockDim.x + s])[tid];
        }
        __syncthreads();
    }

    if (tid == 0) {
        out_min_sdf[blockIdx.x] = sdata[0];
        out_max_sdf[blockIdx.x] = sdata[blockDim.x];
        out_avg_sdf[blockIdx.x] = sdata[blockDim.x * 2];
        out_solid_count[blockIdx.x] = ((int*)&sdata[blockDim.x * 3])[0];
        out_channel_count[blockIdx.x] = ((int*)&sdata[blockDim.x * 3 + blockDim.x])[0];
    }
}

} // namespace cuda_tools
} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_CUDA_ROCKET_KERNELS_H_HAS_BEEN_INCLUDED
