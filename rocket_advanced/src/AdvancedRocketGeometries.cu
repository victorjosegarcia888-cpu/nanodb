#include "AdvancedRocketGeometries.cu.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace rocket_advanced {
namespace cuda {

// ============================================================================
// SDF callbacks for RenderImplicit-style rendering
// ============================================================================

struct AerospikeUserData {
    AerospikePlug params;
};

__device__ float aerospikeSdfCallback(float x, float y, float z, void* user_data) {
    AerospikeUserData* data = (AerospikeUserData*)user_data;
    const AerospikePlug& p = data->params;

    float plug = sdAerospikePlug(x, y, z, (float)p.throat_radius, (float)p.nu_exit,
                                 (float)p.spike_length, (float)p.base_radius);

    // Cowl subtraction (outer cylinder)
    float cowl = sdCylinderZ(x, y, z, 0.0f, 0.0f, (float)p.cowl_radius);

    // Gyroid channels carved into plug
    float gyroid = sdGyroid(x, y, z, (float)p.gyroid_scale, (float)p.gyroid_width, 0.0f);

    // Composite: plug minus gyroid, minus cowl
    float result = fmaxf(fmaxf(plug, -gyroid), -cowl);
    return result;
}

struct LatticeInjectorUserData {
    LatticeInjectorFace params;
    float* beam_starts;
    float* beam_ends;
    float* beam_radii;
    int num_beams;
};

__device__ float latticeInjectorSdfCallback(float x, float y, float z, void* user_data) {
    LatticeInjectorUserData* data = (LatticeInjectorUserData*)user_data;
    const LatticeInjectorFace& p = data->params;

    // Base cylinder
    float base = sdCylinderZ(x, y, z, 0.0f, 0.0f, (float)p.face_radius);

    // Subtract injector holes
    float holes = 1e6f;
    for (int i = 0; i < data->num_beams; ++i) {
        float hx = data->beam_starts[i * 3];
        float hy = data->beam_starts[i * 3 + 1];
        float hz = data->beam_starts[i * 3 + 2];
        float hr = sdCylinderZ(x, y, z, hx, hy, (float)p.hole_radius);
        holes = fminf(holes, hr);
    }

    // Lattice beams
    float beams = 1e6f;
    for (int i = 0; i < data->num_beams; ++i) {
        float db = sdBeam(x, y, z,
                          data->beam_starts[i * 3], data->beam_starts[i * 3 + 1], data->beam_starts[i * 3 + 2],
                          data->beam_ends[i * 3], data->beam_ends[i * 3 + 1], data->beam_ends[i * 3 + 2],
                          data->beam_radii[i]);
        beams = fminf(beams, db);
    }

    return fmaxf(fmaxf(base, -holes), beams);
}

struct RegenerativeJacketUserData {
    RegenerativeJacket params;
};

__device__ float regenerativeJacketSdfCallback(float x, float y, float z, void* user_data) {
    RegenerativeJacketUserData* data = (RegenerativeJacketUserData*)user_data;
    const RegenerativeJacket& p = data->params;

    // Outer cylinder
    float outer = sdCylinderZ(x, y, z, 0.0f, 0.0f, (float)p.outer_radius);
    // Inner cylinder
    float inner = sdCylinderZ(x, y, z, 0.0f, 0.0f, (float)p.inner_radius);

    // Gyroid channels between inner and outer
    float gyroid = sdGyroid(x, y, z, (float)p.gyroid_scale, (float)p.channel_width * 0.5f, 0.0f);

    // Jacket = outer minus inner minus gyroid channels
    float wall = fmaxf(outer, -inner);
    return fmaxf(wall, -gyroid);
}

// ============================================================================
// Host-side wrapper implementations
// ============================================================================

} // namespace cuda

// Host wrappers are directly in rocket_advanced namespace

AerospikePlug makeAerospikePlug(const GeometryInputs& geometry,
                                int nx, int ny, int nz, float voxel_size) {
    AerospikePlug params{};
    params.throat_radius = geometry.throat_diameter_m * 0.5;
    params.nu_exit = 1.0;
    params.spike_length = geometry.nozzle_length_m;
    params.base_radius = geometry.chamber_diameter_m * 0.5;
    params.cowl_radius = geometry.exit_diameter_m * 0.5;
    params.gyroid_scale = geometry.throat_diameter_m * 0.15;
    params.gyroid_width = geometry.throat_diameter_m * 0.02;
    params.wall_thickness = geometry.throat_diameter_m * 0.04;
    params.channel_width = geometry.throat_diameter_m * 0.08;
    params.nx = nx;
    params.ny = ny;
    params.nz = nz;
    params.voxel_size = voxel_size;
    return params;
}

bool renderAerospikePlug(float* d_sdf_field, const AerospikePlug& params) {
    VoxelGridDesc desc;
    desc.nx = params.nx;
    desc.ny = params.ny;
    desc.nz = params.nz;
    desc.voxel_size = params.voxel_size;
    desc.origin_x = -params.cowl_radius;
    desc.origin_y = -params.cowl_radius;
    desc.origin_z = 0.0f;

    size_t nvox = (size_t)params.nx * params.ny * params.nz;
    size_t bytes = nvox * sizeof(float);

    cuda::AerospikeUserData host_data;
    host_data.params = params;

    cuda::AerospikeUserData* d_user_data = nullptr;
    cudaError_t err = cudaMalloc(&d_user_data, sizeof(cuda::AerospikeUserData));
    if (err != cudaSuccess) return false;
    cudaMemcpy(d_user_data, &host_data, sizeof(cuda::AerospikeUserData), cudaMemcpyHostToDevice);

    dim3 block(8, 8, 8);
    dim3 grid((params.nx + block.x - 1) / block.x,
              (params.ny + block.y - 1) / block.y,
              (params.nz + block.z - 1) / block.z);

    cuda::renderImplicitSDFKernel<<<grid, block>>>(d_sdf_field, desc,
                                                   cuda::aerospikeSdfCallback, d_user_data);
    err = cudaGetLastError();
    cudaDeviceSynchronize();

    cudaFree(d_user_data);
    return err == cudaSuccess;
}

bool renderLatticeInjectorFace(float* d_sdf_field, const LatticeInjectorFace& params) {
    VoxelGridDesc desc;
    desc.nx = params.nx;
    desc.ny = params.ny;
    desc.nz = params.nz;
    desc.voxel_size = params.voxel_size;
    desc.origin_x = -params.face_radius;
    desc.origin_y = -params.face_radius;
    desc.origin_z = 0.0f;

    // Generate lattice beams (simplified hexagonal pattern)
    std::vector<float> beam_starts, beam_ends, beam_radii;
    int num_beams = 0;

    float pitch = (float)params.hole_pitch;
    float r_face = (float)params.face_radius;
    float r_hole = (float)params.hole_radius;
    float r_beam_min = (float)params.beam_radius_min;
    float r_beam_max = (float)params.beam_radius_max;

    for (float angle = 0.0f; angle < 2.0f * M_PI; angle += M_PI / 6.0f) {
        float cx = cosf(angle) * r_face * 0.5f;
        float cy = sinf(angle) * r_face * 0.5f;
        float cz = 0.0f;

        // Beam from center to periphery
        beam_starts.push_back(0.0f);
        beam_starts.push_back(0.0f);
        beam_starts.push_back(0.0f);
        beam_ends.push_back(cx);
        beam_ends.push_back(cy);
        beam_ends.push_back(cz + (float)params.beam_length);
        beam_radii.push_back(r_beam_min);
        num_beams++;

        // Beam around circumference
        float nx = cosf(angle + M_PI / 6.0f) * r_face;
        float ny = sinf(angle + M_PI / 6.0f) * r_face;
        beam_starts.push_back(cx);
        beam_starts.push_back(cy);
        beam_starts.push_back(cz);
        beam_ends.push_back(nx);
        beam_ends.push_back(ny);
        beam_ends.push_back(cz + (float)params.beam_length);
        beam_radii.push_back(r_beam_max);
        num_beams++;
    }

    float *d_starts = nullptr, *d_ends = nullptr, *d_radii = nullptr;
    cudaMalloc(&d_starts, num_beams * 3 * sizeof(float));
    cudaMalloc(&d_ends, num_beams * 3 * sizeof(float));
    cudaMalloc(&d_radii, num_beams * sizeof(float));
    cudaMemcpy(d_starts, beam_starts.data(), num_beams * 3 * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_ends, beam_ends.data(), num_beams * 3 * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_radii, beam_radii.data(), num_beams * sizeof(float), cudaMemcpyHostToDevice);

    dim3 block(8, 8, 8);
    dim3 grid((params.nx + block.x - 1) / block.x,
              (params.ny + block.y - 1) / block.y,
              (params.nz + block.z - 1) / block.z);

    cuda::renderLatticeBeamsKernel<<<grid, block>>>(d_sdf_field, desc,
                                                    d_starts, d_ends, d_radii, num_beams);
    cudaDeviceSynchronize();

    cudaFree(d_starts);
    cudaFree(d_ends);
    cudaFree(d_radii);

    return true;
}

bool renderRegenerativeJacket(float* d_sdf_field, const RegenerativeJacket& params) {
    VoxelGridDesc desc;
    desc.nx = params.nx;
    desc.ny = params.ny;
    desc.nz = params.nz;
    desc.voxel_size = params.voxel_size;
    desc.origin_x = -params.outer_radius;
    desc.origin_y = -params.outer_radius;
    desc.origin_z = 0.0f;

    cuda::RegenerativeJacketUserData host_data;
    host_data.params = params;

    cuda::RegenerativeJacketUserData* d_user_data = nullptr;
    cudaError_t err = cudaMalloc(&d_user_data, sizeof(cuda::RegenerativeJacketUserData));
    if (err != cudaSuccess) return false;
    cudaMemcpy(d_user_data, &host_data, sizeof(cuda::RegenerativeJacketUserData), cudaMemcpyHostToDevice);

    dim3 block(8, 8, 8);
    dim3 grid((params.nx + block.x - 1) / block.x,
              (params.ny + block.y - 1) / block.y,
              (params.nz + block.z - 1) / block.z);

    cuda::renderImplicitSDFKernel<<<grid, block>>>(d_sdf_field, desc,
                                                   cuda::regenerativeJacketSdfCallback, d_user_data);
    err = cudaGetLastError();
    cudaDeviceSynchronize();

    cudaFree(d_user_data);
    return err == cudaSuccess;
}

bool performCSGOnDevice(float* d_result, const float* d_a, const float* d_b,
                        int nx, int ny, int nz, const char* operation) {
    int op = 0;
    if (strcmp(operation, "subtract") == 0) op = 1;
    else if (strcmp(operation, "intersect") == 0) op = 2;
    else if (strcmp(operation, "xor") == 0) op = 3;

    dim3 block(8, 8, 8);
    dim3 grid((nx + block.x - 1) / block.x,
              (ny + block.y - 1) / block.y,
              (nz + block.z - 1) / block.z);

    cuda::csgOperationKernel<<<grid, block>>>(d_result, d_a, d_b, nx, ny, nz, op);
    cudaDeviceSynchronize();

    return true;
}

std::vector<int> detectOverhangRegions(const float* d_sdf_field, const VoxelGridDesc& desc,
                                       float max_overhang_angle_deg) {
    size_t nvox = (size_t)desc.nx * desc.ny * desc.nz;
    uint8_t* d_overhang = nullptr;
    cudaMalloc(&d_overhang, nvox * sizeof(uint8_t));
    cudaMemset(d_overhang, 0, nvox * sizeof(uint8_t));

    dim3 block(8, 8, 8);
    dim3 grid((desc.nx + block.x - 1) / block.x,
              (desc.ny + block.y - 1) / block.y,
              (desc.nz + block.z - 1) / block.z);

    cuda::detectOverhangsKernel<<<grid, block>>>(d_overhang, d_sdf_field,
                                                 desc.nx, desc.ny, desc.nz,
                                                 max_overhang_angle_deg);
    cudaDeviceSynchronize();

    uint8_t* h_overhang = new uint8_t[nvox];
    cudaMemcpy(h_overhang, d_overhang, nvox * sizeof(uint8_t), cudaMemcpyDeviceToHost);

    std::vector<int> overhang_indices;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_overhang[i]) overhang_indices.push_back((int)i);
    }

    delete[] h_overhang;
    cudaFree(d_overhang);
    return overhang_indices;
}

bool exportManufacturingMetaJSON(const std::string& path, const ManufacturingMeta& meta) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "{\n";
    out << "  \"material_name\": \"" << meta.material_name << "\",\n";
    out << "  \"printer_id\": \"" << meta.printer_id << "\",\n";
    out << "  \"layer_thickness_um\": " << meta.layer_thickness_um << ",\n";
    out << "  \"laser_power_W\": " << meta.laser_power_W << ",\n";
    out << "  \"scan_speed_mm_s\": " << meta.scan_speed_mm_s << ",\n";
    out << "  \"hatch_spacing_mm\": " << meta.hatch_spacing_mm << ",\n";
    out << "  \"requires_support\": " << (meta.requires_support ? "true" : "false") << ",\n";
    out << "  \"requires_chamber_inert\": " << (meta.requires_chamber_inert ? "true" : "false") << ",\n";
    out << "  \"notes\": \"" << meta.notes << "\"\n";
    out << "}\n";
    out.close();
    return true;
}

ThermalStats computeThermalStats(const float* d_temperature, const float* d_gradient_magnitude,
                                 int nx, int ny, int nz) {
    ThermalStats stats = {};
    stats.max_gradient = -1e30f;
    stats.avg_gradient = 0.0f;
    stats.max_temperature = -1e30f;
    stats.min_temperature = 1e30f;

    size_t nvox = (size_t)nx * ny * nz;
    float* h_temp = new float[nvox];
    float* h_grad = new float[nvox];
    cudaMemcpy(h_temp, d_temperature, nvox * sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_grad, d_gradient_magnitude, nvox * sizeof(float), cudaMemcpyDeviceToHost);

    for (size_t i = 0; i < nvox; ++i) {
        if (h_temp[i] < stats.min_temperature) stats.min_temperature = h_temp[i];
        if (h_temp[i] > stats.max_temperature) stats.max_temperature = h_temp[i];
        if (h_grad[i] > stats.max_gradient) stats.max_gradient = h_grad[i];
        stats.avg_gradient += h_grad[i];
    }
    stats.avg_gradient /= nvox;

    delete[] h_temp;
    delete[] h_grad;
    return stats;
}

bool renderMultiResAssembly(float* d_combined_sdf, const MultiResAssembly& assembly) {
    // Simplified: zero out combined field
    size_t total_vox = 0;
    for (const auto& part : assembly.parts) {
        total_vox += (size_t)part.bbox.x_max * part.bbox.y_max * part.bbox.z_max;
    }
    cudaMemset(d_combined_sdf, 0, total_vox * sizeof(float));
    return true;
}

bool extractMeshToSTL(const float* d_sdf_field, const VoxelGridDesc& desc,
                      const std::string& output_path, int max_vertices) {
    std::ofstream out(output_path);
    if (!out.is_open()) return false;

    out << "solid rocket_advanced\n";

    size_t nvox = (size_t)desc.nx * desc.ny * desc.nz;
    float* h_sdf = new float[nvox];
    cudaMemcpy(h_sdf, d_sdf_field, nvox * sizeof(float), cudaMemcpyDeviceToHost);

    int triangle_count = 0;
    for (int iz = 0; iz < desc.nz - 1; ++iz) {
        for (int iy = 0; iy < desc.ny - 1; ++iy) {
            for (int ix = 0; ix < desc.nx - 1; ++ix) {
                int idx = ix + iy * desc.nx + iz * desc.nx * desc.ny;
                float d = h_sdf[idx];

                if (d < 0.0f && triangle_count < max_vertices / 3) {
                    float x = desc.origin_x + ix * desc.voxel_size;
                    float y = desc.origin_y + iy * desc.voxel_size;
                    float z = desc.origin_z + iz * desc.voxel_size;

                    // Simple cube face output (placeholder for proper marching cubes)
                    out << "facet normal 0 0 1\n";
                    out << "  outer loop\n";
                    out << "    vertex " << x << " " << y << " " << z << "\n";
                    out << "    vertex " << x + desc.voxel_size << " " << y << " " << z << "\n";
                    out << "    vertex " << x << " " << y + desc.voxel_size << " " << z << "\n";
                    out << "  endloop\n";
                    out << "endfacet\n";
                    triangle_count++;
                }
            }
        }
    }

    out << "endsolid rocket_advanced\n";
    out.close();

    delete[] h_sdf;
    return true;
}

} // namespace rocket_advanced
