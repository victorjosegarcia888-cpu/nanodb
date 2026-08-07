#ifndef ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_CU_H_HAS_BEEN_INCLUDED
#define ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_CU_H_HAS_BEEN_INCLUDED

#include "AdvancedRocketGeometries.h"
#include <cuda_runtime_api.h>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace rocket_advanced {
namespace cuda {

// ============================================================================
// SDF primitives (inspired by PicoGKRuntime Shapes)
// ============================================================================

__device__ float sdSphere(float x, float y, float z, float cx, float cy, float cz, float r) {
    float dx = x - cx, dy = y - cy, dz = z - cz;
    return sqrtf(dx*dx + dy*dy + dz*dz) - r;
}

__device__ float sdCylinderZ(float x, float y, float z, float cx, float cy, float r) {
    return sqrtf((x - cx)*(x - cx) + (y - cy)*(y - cy)) - r;
}

__device__ float sdTorus(float x, float y, float z, float cx, float cy, float cz,
                         float R, float r) {
    float dx = x - cx, dy = y - cy, dz = z - cz;
    float q = sqrtf(dx*dx + dy*dy) - R;
    return sqrtf(q*q + dz*dz) - r;
}

__device__ float sdBox(float x, float y, float z, float bx, float by, float bz) {
    float dx = fabsf(x) - bx, dy = fabsf(y) - by, dz = fabsf(z) - bz;
    return fminf(fmaxf(dx, fmaxf(dy, dz)), 0.0f) + sqrtf(dx*dx + dy*dy + dz*dz);
}

__device__ float sdGyroid(float x, float y, float z, float scale, float half_width, float z_offset) {
    float gx = sinf(2.0f * M_PI * (x + z_offset) / scale)
             + sinf(2.0f * M_PI * y / scale)
             + sinf(2.0f * M_PI * z / scale);
    return gx - half_width;
}

__device__ float sdAerospikePlug(float x, float y, float z, float throat_r, float nu_exit,
                                 float spike_length, float base_r) {
    if (z < 0.0f || z > spike_length) return 1e6f;
    float r = sqrtf(x*x + y*y);
    float k = tanf(nu_exit) * 0.1f;
    float r_spike = throat_r - k * z;
    if (r_spike < 0.05f * throat_r) r_spike = 0.05f * throat_r;
    return r - r_spike;
}

__device__ float sdBeam(float x, float y, float z, float x1, float y1, float z1,
                        float x2, float y2, float z2, float radius) {
    float ax = x2 - x1, ay = y2 - y1, az = z2 - z1;
    float ab2 = ax*ax + ay*ay + az*az;
    float apx = x - x1, apy = y - y1, apz = z - z1;
    float t = fminf(fmaxf((apx*ax + apy*ay + apz*az) / (ab2 + 1e-12f), 0.0f), 1.0f);
    float projx = x1 + t * ax, projy = y1 + t * ay, projz = z1 + t * az;
    float dx = x - projx, dy = y - projy, dz = z - projz;
    return sqrtf(dx*dx + dy*dy + dz*dz) - radius;
}

// ============================================================================
// Kernel implementations
// ============================================================================

__global__ void renderImplicitSDFKernel(float* sdf_field, const VoxelGridDesc desc,
                                        SdfFunction sdf, void* user_data) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= desc.nx || iy >= desc.ny || iz >= desc.nz) return;

    float x = desc.origin_x + ix * desc.voxel_size;
    float y = desc.origin_y + iy * desc.voxel_size;
    float z = desc.origin_z + iz * desc.voxel_size;

    int idx = ix + iy * desc.nx + iz * desc.nx * desc.ny;
    sdf_field[idx] = sdf(x, y, z, user_data);
}

__global__ void csgOperationKernel(float* result, const float* field_a, const float* field_b,
                                   int nx, int ny, int nz, int op) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float a = field_a[idx];
    float b = field_b[idx];
    float r = 0.0f;

    if (op == 0) {
        // Union
        r = fminf(a, b);
    } else if (op == 1) {
        // Subtract A - B
        r = fmaxf(a, -b);
    } else if (op == 2) {
        // Intersect
        r = fmaxf(a, b);
    } else {
        // XOR-like
        r = fminf(fmaxf(a, b), fmaxf(-a, -b));
    }

    result[idx] = r;
}

__global__ void offsetFieldKernel(float* offset_field, const float* sdf_field,
                                  int nx, int ny, int nz, float offset_distance) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    offset_field[idx] = sdf_field[idx] - offset_distance;
}

__global__ void renderLatticeBeamsKernel(float* sdf_field, const VoxelGridDesc desc,
                                         const float* beam_starts, const float* beam_ends,
                                         const float* beam_radii, int num_beams) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= desc.nx || iy >= desc.ny || iz >= desc.nz) return;

    float x = desc.origin_x + ix * desc.voxel_size;
    float y = desc.origin_y + iy * desc.voxel_size;
    float z = desc.origin_z + iz * desc.voxel_size;

    int idx = ix + iy * desc.nx + iz * desc.nx * desc.ny;
    float d = 1e6f;

    for (int b = 0; b < num_beams; ++b) {
        float x1 = beam_starts[b * 3];
        float y1 = beam_starts[b * 3 + 1];
        float z1 = beam_starts[b * 3 + 2];
        float x2 = beam_ends[b * 3];
        float y2 = beam_ends[b * 3 + 1];
        float z2 = beam_ends[b * 3 + 2];
        float r = beam_radii[b];

        float db = sdBeam(x, y, z, x1, y1, z1, x2, y2, z2, r);
        d = fminf(d, db);
    }

    sdf_field[idx] = d;
}

__global__ void renderGyroidChannelsKernel(float* sdf_field, const VoxelGridDesc desc,
                                           float scale, float half_width, float z_offset) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= desc.nx || iy >= desc.ny || iz >= desc.nz) return;

    float x = desc.origin_x + ix * desc.voxel_size;
    float y = desc.origin_y + iy * desc.voxel_size;
    float z = desc.origin_z + iz * desc.voxel_size;

    int idx = ix + iy * desc.nx + iz * desc.nx * desc.ny;
    sdf_field[idx] = sdGyroid(x, y, z, scale, half_width, z_offset);
}

__global__ void detectOverhangsKernel(uint8_t* overhang_mask, const float* sdf_field,
                                      int nx, int ny, int nz, float max_overhang_angle_deg) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float d = sdf_field[idx];

    // Simplified overhang: if surface normal points downward and angle exceeds threshold
    // In practice, compute gradient from neighboring voxels
    bool overhang = false;
    if (d < 0.0f && iz > 0) {
        int idx_below = ix + iy * nx + (iz - 1) * nx * ny;
        if (sdf_field[idx_below] > d + 0.001f) {
            overhang = true;
        }
    }

    overhang_mask[idx] = overhang ? 1 : 0;
}

__global__ void extractSurfaceMeshKernel(uint32_t* vertex_buffer, uint32_t* index_buffer,
                                         int* out_vertex_count, int* out_triangle_count,
                                         const float* sdf_field, int nx, int ny, int nz,
                                         float voxel_size, int max_vertices, int max_triangles) {
    // Simplified: output one triangle per surface crossing along X axis
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx - 1 || iy >= ny || iz >= nz) return;

    int idx0 = ix + iy * nx + iz * nx * ny;
    int idx1 = (ix + 1) + iy * nx + iz * nx * ny;

    float d0 = sdf_field[idx0];
    float d1 = sdf_field[idx1];

    // Surface crossing
    if ((d0 < 0.0f) != (d1 < 0.0f)) {
        float t = fabsf(d0) / (fabsf(d0) + fabsf(d1) + 1e-12f);
        float x = (ix + t) * voxel_size;
        float y = iy * voxel_size;
        float z = iz * voxel_size;

        // Atomic add for vertex count (simplified)
        int vidx = atomicAdd(out_vertex_count, 1);
        if (vidx < max_vertices) {
            vertex_buffer[vidx * 3] = __float2int_rn(x * 1000.0f);
            vertex_buffer[vidx * 3 + 1] = __float2int_rn(y * 1000.0f);
            vertex_buffer[vidx * 3 + 2] = __float2int_rn(z * 1000.0f);
        }
    }
}

__global__ void computeThermalGradientKernel(float* grad_x, float* grad_y, float* grad_z,
                                             const float* temperature_field,
                                             int nx, int ny, int nz, float voxel_size) {
    int ix = blockIdx.x * blockDim.x + threadIdx.x;
    int iy = blockIdx.y * blockDim.y + threadIdx.y;
    int iz = blockIdx.z * blockDim.z + threadIdx.z;

    if (ix >= nx || iy >= ny || iz >= nz) return;

    int idx = ix + iy * nx + iz * nx * ny;
    float T = temperature_field[idx];

    float Tx = 0.0f, Ty = 0.0f, Tz = 0.0f;

    if (ix > 0 && ix < nx - 1) {
        Tx = (temperature_field[idx + 1] - temperature_field[idx - 1]) / (2.0f * voxel_size);
    } else if (ix == 0 && nx > 1) {
        Tx = (temperature_field[idx + 1] - T) / voxel_size;
    } else if (ix == nx - 1 && nx > 1) {
        Tx = (T - temperature_field[idx - 1]) / voxel_size;
    }

    if (iy > 0 && iy < ny - 1) {
        Ty = (temperature_field[idx + nx] - temperature_field[idx - nx]) / (2.0f * voxel_size);
    } else if (iy == 0 && ny > 1) {
        Ty = (temperature_field[idx + nx] - T) / voxel_size;
    } else if (iy == ny - 1 && ny > 1) {
        Ty = (T - temperature_field[idx - nx]) / voxel_size;
    }

    if (iz > 0 && iz < nz - 1) {
        Tz = (temperature_field[idx + nx * ny] - temperature_field[idx - nx * ny]) / (2.0f * voxel_size);
    } else if (iz == 0 && nz > 1) {
        Tz = (temperature_field[idx + nx * ny] - T) / voxel_size;
    } else if (iz == nz - 1 && nz > 1) {
        Tz = (T - temperature_field[idx - nx * ny]) / voxel_size;
    }

    grad_x[idx] = Tx;
    grad_y[idx] = Ty;
    grad_z[idx] = Tz;
}

} // namespace cuda
} // namespace rocket_advanced

#endif // ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_CU_H_HAS_BEEN_INCLUDED
