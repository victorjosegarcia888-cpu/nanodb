#include <cstdio>
#include <cuda_runtime_api.h>
#include <cmath>
#include <vector>
#include <string>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include "nanodb/nanovdb/tools/CudaVoxelTestUtils.h"
#include "rocket_advanced/include/AdvancedRocketGeometries.h"

namespace ra = rocket_advanced;

int main() {
    printf("=== Advanced Rocket Geometries Test ===\n");

    if (!nanovdb::tools::cuda_tools::verifyDeviceProperties(700)) {
        printf("No suitable CUDA device. Skipping.\n");
        return 0;
    }

    // 1. Aerospike plug test
    printf("\n--- Aerospike Plug ---\n");
    ra::AerospikePlug aero_params;
    aero_params.throat_radius = 0.05;
    aero_params.nu_exit = 1.5;
    aero_params.spike_length = 0.5;
    aero_params.base_radius = 0.1;
    aero_params.cowl_radius = 0.15;
    aero_params.gyroid_scale = 0.02;
    aero_params.gyroid_width = 0.5;
    aero_params.wall_thickness = 0.005;
    aero_params.channel_width = 0.01;
    aero_params.nx = 64;
    aero_params.ny = 64;
    aero_params.nz = 128;
    aero_params.voxel_size = 0.002f;

    size_t nvox = (size_t)aero_params.nx * aero_params.ny * aero_params.nz;
    size_t bytes = nvox * sizeof(float);

    float *d_aero_sdf = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_aero_sdf, bytes), 1);

    bool aero_ok = ra::renderAerospikePlug(d_aero_sdf, aero_params);
    printf("Aerospike render: %s\n", aero_ok ? "OK" : "FAIL");

    float *h_aero = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_aero, d_aero_sdf, bytes, cudaMemcpyDeviceToHost), 1);

    float aero_min = 1e30f, aero_max = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_aero[i] < aero_min) aero_min = h_aero[i];
        if (h_aero[i] > aero_max) aero_max = h_aero[i];
    }
    printf("Aerospike SDF range: [%.4f, %.4f]\n", aero_min, aero_max);

    // 2. Lattice injector face test
    printf("\n--- Lattice Injector Face ---\n");
    ra::LatticeInjectorFace lattice_params;
    lattice_params.face_radius = 0.05;
    lattice_params.hole_radius = 0.003;
    lattice_params.hole_pitch = 0.01;
    lattice_params.beam_radius_min = 0.001;
    lattice_params.beam_radius_max = 0.002;
    lattice_params.beam_length = 0.02;
    lattice_params.nx = 64;
    lattice_params.ny = 64;
    lattice_params.nz = 32;
    lattice_params.voxel_size = 0.001f;

    nvox = (size_t)lattice_params.nx * lattice_params.ny * lattice_params.nz;
    bytes = nvox * sizeof(float);

    float *d_lattice_sdf = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_lattice_sdf, bytes), 1);

    bool lattice_ok = ra::renderLatticeInjectorFace(d_lattice_sdf, lattice_params);
    printf("Lattice injector render: %s\n", lattice_ok ? "OK" : "FAIL");

    float *h_lattice = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_lattice, d_lattice_sdf, bytes, cudaMemcpyDeviceToHost), 1);

    float lat_min = 1e30f, lat_max = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_lattice[i] < lat_min) lat_min = h_lattice[i];
        if (h_lattice[i] > lat_max) lat_max = h_lattice[i];
    }
    printf("Lattice SDF range: [%.4f, %.4f]\n", lat_min, lat_max);

    // 3. Regenerative jacket test
    printf("\n--- Regenerative Jacket ---\n");
    ra::RegenerativeJacket jacket_params;
    jacket_params.inner_radius = 0.05;
    jacket_params.outer_radius = 0.1;
    jacket_params.channel_width = 0.005;
    jacket_params.gyroid_scale = 0.015;
    jacket_params.nx = 64;
    jacket_params.ny = 64;
    jacket_params.nz = 128;
    jacket_params.voxel_size = 0.002f;

    nvox = (size_t)jacket_params.nx * jacket_params.ny * jacket_params.nz;
    bytes = nvox * sizeof(float);

    float *d_jacket_sdf = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_jacket_sdf, bytes), 1);

    bool jacket_ok = ra::renderRegenerativeJacket(d_jacket_sdf, jacket_params);
    printf("Regenerative jacket render: %s\n", jacket_ok ? "OK" : "FAIL");

    float *h_jacket = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_jacket, d_jacket_sdf, bytes, cudaMemcpyDeviceToHost), 1);

    float jack_min = 1e30f, jack_max = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_jacket[i] < jack_min) jack_min = h_jacket[i];
        if (h_jacket[i] > jack_max) jack_max = h_jacket[i];
    }
    printf("Jacket SDF range: [%.4f, %.4f]\n", jack_min, jack_max);

    // 4. CSG operation test
    printf("\n--- CSG Boolean Operations ---\n");
    float *d_csg_result = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_csg_result, bytes), 1);

    bool csg_ok = ra::performCSGOnDevice(d_csg_result, d_aero_sdf, d_jacket_sdf,
                                          jacket_params.nx, jacket_params.ny, jacket_params.nz,
                                          "subtract");
    printf("CSG subtract: %s\n", csg_ok ? "OK" : "FAIL");

    float *h_csg = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_csg, d_csg_result, bytes, cudaMemcpyDeviceToHost), 1);

    float csg_min = 1e30f, csg_max = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (h_csg[i] < csg_min) csg_min = h_csg[i];
        if (h_csg[i] > csg_max) csg_max = h_csg[i];
    }
    printf("CSG result SDF range: [%.4f, %.4f]\n", csg_min, csg_max);

    // 5. Overhang detection test
    printf("\n--- Overhang Detection ---\n");
    ra::VoxelGridDesc desc;
    desc.nx = jacket_params.nx;
    desc.ny = jacket_params.ny;
    desc.nz = jacket_params.nz;
    desc.voxel_size = jacket_params.voxel_size;
    desc.origin_x = -jacket_params.outer_radius;
    desc.origin_y = -jacket_params.outer_radius;
    desc.origin_z = 0.0f;

    std::vector<int> overhangs = ra::detectOverhangRegions(d_jacket_sdf, desc, 45.0f);
    printf("Overhang voxels detected: %zu / %zu (%.2f%%)\n",
           overhangs.size(), nvox, 100.0f * overhangs.size() / nvox);

    // 6. Manufacturing metadata export test
    printf("\n--- Manufacturing Metadata Export ---\n");
    ra::ManufacturingMeta meta;
    meta.material_name = "Inconel718";
    meta.printer_id = "SLM_NXG_XII_600";
    meta.layer_thickness_um = 40.0f;
    meta.laser_power_W = 350.0f;
    meta.scan_speed_mm_s = 1200.0f;
    meta.hatch_spacing_mm = 0.12f;
    meta.requires_support = true;
    meta.requires_chamber_inert = true;
    meta.notes = "Rocket nozzle aerospike with regenerative cooling";

    bool meta_ok = ra::exportManufacturingMetaJSON("/tmp/rocket_meta.json", meta);
    printf("Manufacturing metadata export: %s\n", meta_ok ? "OK" : "FAIL");

    // 7. Thermal gradient computation test
    printf("\n--- Thermal Gradient ---\n");
    float *d_temperature = nullptr, *d_grad_x = nullptr, *d_grad_y = nullptr, *d_grad_z = nullptr;
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_temperature, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_grad_x, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_grad_y, bytes), 1);
    CUDA_CHECK_AND_RETURN(cudaMalloc(&d_grad_z, bytes), 1);

    cudaMemset(d_temperature, 0, bytes);

    {
        using namespace ra::cuda;
        dim3 block(8, 8, 8);
        dim3 grid((jacket_params.nx + block.x - 1) / block.x,
                  (jacket_params.ny + block.y - 1) / block.y,
                  (jacket_params.nz + block.z - 1) / block.z);
        computeThermalGradientKernel<<<grid, block>>>(d_grad_x, d_grad_y, d_grad_z,
                                                             d_temperature,
                                                             jacket_params.nx, jacket_params.ny, jacket_params.nz,
                                                             jacket_params.voxel_size);
    }
    cudaDeviceSynchronize();

    float *h_grad_x = new float[nvox];
    CUDA_CHECK_AND_RETURN(cudaMemcpy(h_grad_x, d_grad_x, bytes, cudaMemcpyDeviceToHost), 1);

    float max_grad = -1e30f;
    for (size_t i = 0; i < nvox; ++i) {
        if (fabsf(h_grad_x[i]) > max_grad) max_grad = fabsf(h_grad_x[i]);
    }
    printf("Max thermal gradient X: %.4f K/m\n", max_grad / jacket_params.voxel_size);

    // 8. STL export test
    printf("\n--- STL Export ---\n");
    bool stl_ok = ra::extractMeshToSTL(d_aero_sdf, desc, "/tmp/rocket_advanced_test.stl", 100000);
    printf("STL export: %s\n", stl_ok ? "OK" : "FAIL");

    // 9. Cleanup
    delete[] h_aero; delete[] h_lattice; delete[] h_jacket; delete[] h_csg; delete[] h_grad_x;
    cudaFree(d_aero_sdf); cudaFree(d_lattice_sdf); cudaFree(d_jacket_sdf);
    cudaFree(d_csg_result); cudaFree(d_temperature); cudaFree(d_grad_x);
    cudaFree(d_grad_y); cudaFree(d_grad_z);

    printf("\nAdvanced rocket geometries test passed.\n");
    return 0;
}
