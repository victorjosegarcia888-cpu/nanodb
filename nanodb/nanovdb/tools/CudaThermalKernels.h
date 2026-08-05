#ifndef NANOVDB_TOOLS_CUDA_THERMAL_KERNELS_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_CUDA_THERMAL_KERNELS_H_HAS_BEEN_INCLUDED

#include <cuda_runtime_api.h>
#include <cstdio>
#include <cmath>

namespace nanovdb {
namespace tools {
namespace cuda_tools {

/// @brief Material properties for thermal analysis on device
struct DeviceThermalParams {
    double k_w;      ///< Thermal conductivity [W/(m·K)]
    double rho_w;    ///< Density [kg/m³]
    double cp_w;     ///< Specific heat [J/(kg·K)]
    double alpha_te; ///< Thermal expansion [1/K]
    double E;        ///< Young's modulus [Pa]
    double nu;       ///< Poisson ratio
    double T_i;      ///< Initial temperature [K]
    double T_aw;     ///< Adiabatic wall temperature [K]
    double h_g;      ///< Gas-side HTC [W/(m²·K)]
};

/// @brief CUDA kernel: transient 1D thermal conduction into a semi-infinite solid.
///        Each thread computes T(z,t) at its voxel coordinate.
__global__ void thermalShockKernel(float* T_field, float* stress_field,
                                   const DeviceThermalParams params,
                                   int nx, int ny, int nz,
                                   float voxel_size, float t_snap,
                                   float z_world_min, float z_world_max) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    float z = z_world_min + (iz / (float)(nz - 1)) * (z_world_max - z_world_min);
    float alpha = (float)(params.k_w / (params.rho_w * params.cp_w));
    float beta = (float)(params.h_g) * sqrtf(alpha * t_snap) / (float)(params.k_w);
    float rhs = 1.0f - expf(beta * beta) * erfc(beta);
    float T_gas = (float)(params.T_i) + (float)(params.T_aw - params.T_i) * rhs;

    float dT = T_gas - (float)(params.T_i);
    float sigma = - (float)(params.E * params.alpha_te * dT) / (1.0f - (float)(params.nu));

    int idx = ix + iy * nx + iz * nx * ny;
    T_field[idx] = T_gas;
    stress_field[idx] = sigma;
}

/// @brief CUDA kernel: heat flux boundary layer along Z
__global__ void heatFluxKernel(float* q_field, int nx, int ny, int nz,
                               float h_g, float T_aw, float T_surface,
                               float z_world_min, float z_world_max) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    float z = z_world_min + (iz / (float)(nz - 1)) * (z_world_max - z_world_min);
    float q = h_g * (T_aw - T_surface);
    int idx = ix + iy * nx + iz * nx * ny;
    q_field[idx] = q;
}

/// @brief CUDA kernel: thermal penetration depth mask
__global__ void penetrationDepthKernel(float* penetration_mask, int nx, int ny, int nz,
                                       float voxel_size, float alpha, float t_snap,
                                       float z_world_min, float z_world_max) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    float z = z_world_min + (iz / (float)(nz - 1)) * (z_world_max - z_world_min);
    float delta_th = 2.0f * sqrtf(alpha * t_snap);
    float mask = (z <= delta_th) ? 1.0f : 0.0f;

    int idx = ix + iy * nx + iz * nx * ny;
    penetration_mask[idx] = mask;
}

/// @brief CUDA kernel: material-specific thermal safety factor map
__global__ void thermalSafetyFactorKernel(float* safety_factor, int nx, int ny, int nz,
                                          const float* stress_field, const float* T_field,
                                          float yield_strength_Pa, int nx_ny) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float sigma = stress_field[idx];
    float T = T_field[idx];

    float safety = yield_strength_Pa / (fabsf(sigma) + 1e-6f);
    safety_factor[idx] = safety;
}

/// @brief Reduction kernel for thermal statistics
__global__ void thermalStatsReduction(const float* T_field, const float* stress_field,
                                      int nvox, float* out_T_min, float* out_T_max,
                                      float* out_T_avg, float* out_sigma_max,
                                      float* out_sigma_avg) {
    extern __shared__ float sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;

    float local_T_min = 1e30f, local_T_max = -1e30f, local_T_sum = 0.0f;
    float local_sigma_max = -1e30f, local_sigma_sum = 0.0f;
    int local_count = 0;

    if (i < nvox) {
        float T = T_field[i];
        float s = stress_field[i];
        local_T_min = T; local_T_max = T; local_T_sum = T;
        local_sigma_max = fabsf(s); local_sigma_sum = fabsf(s);
        local_count = 1;
    }

    sdata[tid] = local_T_min;
    sdata[tid + blockDim.x] = local_T_max;
    sdata[tid + blockDim.x * 2] = local_T_sum;
    sdata[tid + blockDim.x * 3] = local_sigma_max;
    sdata[tid + blockDim.x * 4] = local_sigma_sum;
    sdata[tid + blockDim.x * 5] = local_count;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] = fminf(sdata[tid], sdata[tid + s]);
            sdata[tid + blockDim.x] = fmaxf(sdata[tid + blockDim.x], sdata[tid + blockDim.x + s]);
            sdata[tid + blockDim.x * 2] += sdata[tid + blockDim.x * 2 + s];
            sdata[tid + blockDim.x * 3] = fmaxf(sdata[tid + blockDim.x * 3], sdata[tid + blockDim.x * 3 + s]);
            sdata[tid + blockDim.x * 4] += sdata[tid + blockDim.x * 4 + s];
            sdata[tid + blockDim.x * 5] += sdata[tid + blockDim.x * 5 + s];
        }
        __syncthreads();
    }

    if (tid == 0) {
        out_T_min[blockIdx.x] = sdata[0];
        out_T_max[blockIdx.x] = sdata[blockDim.x];
        out_T_avg[blockIdx.x] = sdata[blockDim.x * 2];
        out_sigma_max[blockIdx.x] = sdata[blockDim.x * 3];
        out_sigma_avg[blockIdx.x] = sdata[blockDim.x * 4];
    }
}

} // namespace cuda_tools
} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_CUDA_THERMAL_KERNELS_H_HAS_BEEN_INCLUDED
