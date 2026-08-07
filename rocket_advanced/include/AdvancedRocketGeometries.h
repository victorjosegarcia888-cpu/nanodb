#ifndef ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_H_HAS_BEEN_INCLUDED
#define ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_H_HAS_BEEN_INCLUDED

#include <cuda_runtime_api.h>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>

namespace rocket_advanced {

/// @brief 3D axis-aligned bounding box
struct BBox3 {
    float x_min, y_min, z_min;
    float x_max, y_max, z_max;

    __host__ __device__ bool contains(float x, float y, float z) const {
        return x >= x_min && x <= x_max &&
               y >= y_min && y <= y_max &&
               z >= z_min && z <= z_max;
    }
};

/// @brief Voxel grid descriptor
struct VoxelGridDesc {
    int nx, ny, nz;
    float voxel_size;
    float origin_x, origin_y, origin_z;
};

/// @brief Signed distance function callback type (inspired by PicoGK PKPFnfSdf)
typedef float (*SdfFunction)(float x, float y, float z, void* user_data);

/// @brief Material ID mapping for manufacturing
struct MaterialMap {
    uint32_t empty : 1;
    uint32_t solid : 1;
    uint32_t channel : 1;
    uint32_t support : 1;
    uint32_t lattice : 1;
    uint32_t reserved : 27;
};

/// @brief Manufacturing metadata (inspired by PicoGK VdbMeta)
struct ManufacturingMeta {
    std::string material_name;
    std::string printer_id;
    float layer_thickness_um;
    float laser_power_W;
    float scan_speed_mm_s;
    float hatch_spacing_mm;
    bool requires_support;
    bool requires_chamber_inert;
    std::string notes;
};

/// @brief Aerospike plug with regenerative gyroid channels
///         Inspirado en: PicoGKRuntime/Source/PicoGKVdbVoxels.h RenderImplicit + ShapeKernel ImplicitGyroid
struct AerospikePlug {
    double throat_radius;
    double nu_exit;
    double spike_length;
    double base_radius;
    double cowl_radius;
    double gyroid_scale;
    double gyroid_width;
    double wall_thickness;
    double channel_width;
    int nx, ny, nz;
    float voxel_size;
};

/// @brief Lattice injector face with tapered beams
///         Inspirado en: PicoGKRuntime/Source/PicoGKLattice.h Lattice::AddBeam
struct LatticeInjectorFace {
    double face_radius;
    double hole_radius;
    double hole_pitch;
    double beam_radius_min;
    double beam_radius_max;
    double beam_length;
    int nx, ny, nz;
    float voxel_size;
};

/// @brief Regenerative cooling jacket with variable thickness
///         Inspirado en: PicoGKRuntime/Source/PicoGKVdbVoxels.h Offset/DoubleOffset
struct RegenerativeJacket {
    double inner_radius;
    double outer_radius;
    double channel_width;
    double gyroid_scale;
    int nx, ny, nz;
    float voxel_size;
};

/// @brief Multi-resolution assembly (workaround for PicoGK single-voxel-size limitation)
///         Inspirado en: PicoGK Discussion #108 + Voxels::roAsMesh
struct MultiResAssembly {
    struct SubAssembly {
        BBox3 bbox;
        float voxel_size;
        std::string name;
    };
    std::vector<SubAssembly> parts;
};

// ============================================================================
// CUDA Kernel declarations (implemented in AdvancedRocketGeometries.cu)
// ============================================================================

namespace cuda {

/// @brief Render an arbitrary SDF into a float voxel field on device.
///         Inspirado en: PicoGKRuntime Voxels::RenderImplicit
__global__ void renderImplicitSDFKernel(float* sdf_field, const VoxelGridDesc desc,
                                        SdfFunction sdf, void* user_data);

/// @brief Boolean CSG operations on two float SDF fields: result = op(a, b)
///         Inspirado en: PicoGKRuntime Voxels::BoolAdd/Subtract/Intersect
__global__ void csgOperationKernel(float* result, const float* field_a, const float* field_b,
                                   int nx, int ny, int nz, int op);

/// @brief Offset a signed distance field by a uniform thickness (shell creation).
///         Inspirado en: PicoGKRuntime Voxels::Offset
__global__ void offsetFieldKernel(float* offset_field, const float* sdf_field,
                                  int nx, int ny, int nz, float offset_distance);

/// @brief Render lattice beams as round cylinders between two points.
///         Inspirado en: PicoGKRuntime Lattice::AddBeam
__global__ void renderLatticeBeamsKernel(float* sdf_field, const VoxelGridDesc desc,
                                         const float* beam_starts, const float* beam_ends,
                                         const float* beam_radii, int num_beams);

/// @brief Render gyroid TPMS channel structure.
///         Inspirado en: LEAP71 ShapeKernel ImplicitGyroid
__global__ void renderGyroidChannelsKernel(float* sdf_field, const VoxelGridDesc desc,
                                           float scale, float half_width, float z_offset);

/// @brief Compute overhang detection for AM support generation.
///         Inspirado en: PicoGK Viewer_EnableGroupWarnOverhang
__global__ void detectOverhangsKernel(uint8_t* overhang_mask, const float* sdf_field,
                                      int nx, int ny, int nz, float max_overhang_angle_deg);

/// @brief Extract surface mesh from SDF using marching cubes-like approach (simplified).
///         Inspirado en: PicoGKRuntime Voxels::roAsMesh + OpenVDB volumeToMesh
__global__ void extractSurfaceMeshKernel(uint32_t* vertex_buffer, uint32_t* index_buffer,
                                         int* out_vertex_count, int* out_triangle_count,
                                         const float* sdf_field, int nx, int ny, int nz,
                                         float voxel_size, int max_vertices, int max_triangles);

/// @brief Embed manufacturing metadata into a NanoVDB-like portable buffer.
///         Inspirado en: PicoGKRuntime VdbMeta
__global__ void embedManufacturingMetaKernel(uint32_t* buffer, uint64_t max_words,
                                             const char* material_name, const char* printer_id,
                                             float layer_thickness, float laser_power);

/// @brief Compute thermal gradient field from temperature field.
///         Inspirado en: PicoGKRuntime VectorField::AddGradientFieldFrom
__global__ void computeThermalGradientKernel(float* grad_x, float* grad_y, float* grad_z,
                                             const float* temperature_field,
                                             int nx, int ny, int nz, float voxel_size);

} // namespace cuda

// ============================================================================
// Host-side wrapper functions (implemented in AdvancedRocketGeometries.cpp)
// ============================================================================

/// @brief Host-side wrapper: render aerospike plug with gyroid channels
bool renderAerospikePlug(float* d_sdf_field, const AerospikePlug& params);

/// @brief Host-side wrapper: render lattice injector face
bool renderLatticeInjectorFace(float* d_sdf_field, const LatticeInjectorFace& params);

/// @brief Host-side wrapper: render regenerative cooling jacket
bool renderRegenerativeJacket(float* d_sdf_field, const RegenerativeJacket& params);

/// @brief Host-side wrapper: perform CSG boolean operation on device
bool performCSGOnDevice(float* d_result, const float* d_a, const float* d_b,
                        int nx, int ny, int nz, const char* operation);

/// @brief Host-side wrapper: extract mesh from device SDF and write STL
bool extractMeshToSTL(const float* d_sdf_field, const VoxelGridDesc& desc,
                      const std::string& output_path, int max_vertices = 1000000);

/// @brief Host-side wrapper: detect overhangs and generate support regions
std::vector<int> detectOverhangRegions(const float* d_sdf_field, const VoxelGridDesc& desc,
                                       float max_overhang_angle_deg = 45.0f);

/// @brief Export manufacturing metadata to JSON sidecar file
bool exportManufacturingMetaJSON(const std::string& path, const ManufacturingMeta& meta);

/// @brief Compute and return thermal gradient magnitude statistics
struct ThermalStats {
    float max_gradient;
    float avg_gradient;
    float max_temperature;
    float min_temperature;
};
ThermalStats computeThermalStats(const float* d_temperature, const float* d_gradient_magnitude,
                                 int nx, int ny, int nz);

/// @brief Multi-resolution assembly: render all sub-assemblies and combine
bool renderMultiResAssembly(float* d_combined_sdf, const MultiResAssembly& assembly);

} // namespace rocket_advanced

#endif // ROCKET_ADVANCED_ADVANCED_ROCKET_GEOMETRIES_H_HAS_BEEN_INCLUDED
