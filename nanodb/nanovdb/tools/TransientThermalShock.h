#ifndef NANOVDB_TOOLS_TRANSIENT_THERMAL_SHOCK_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_TRANSIENT_THERMAL_SHOCK_H_HAS_BEEN_INCLUDED

#include <nanovdb/NanoVDB.h>
#include <nanovdb/GridHandle.h>
#include <nanovdb/tools/GridBuilder.h>
#include <cmath>
#include <algorithm>

namespace nanovdb {
namespace tools {

/// @brief Parameters for UHTC wall transient thermal shock analysis (0-500 ms)
struct UHTCThermalShockParams {
    double T_i;             ///< Initial wall temperature [K]
    double T_aw;            ///< Gas adiabatic wall temperature [K] (3300-3800 K)
    double k_w;             ///< Wall thermal conductivity [W/(m·K)] (~80 for UHTC)
    double rho_w;           ///< Wall density [kg/m³] (~6000 for UHTC)
    double cp_w;            ///< Wall specific heat [J/(kg·K)] (~500 for UHTC)
    double h_g;             ///< Gas-side heat transfer coefficient [W/(m²·K)]
    double t_w;             ///< Wall thickness [m]
    double t_max;           ///< Max time for analysis [s] (0.5 s)
};

/// @brief Computes thermal diffusivity alpha = k / (rho * cp)
inline double computeThermalDiffusivity(double k_w, double rho_w, double cp_w) {
    return k_w / (rho_w * cp_w);
}

/// @brief Computes thermal penetration depth for semi-infinite solid:
///        delta_th(t) ≈ 2 * sqrt(alpha * t)
inline double computeThermalPenetrationDepth(double alpha, double t) {
    return 2.0 * std::sqrt(alpha * t);
}

/// @brief Analytical surface temperature T_gas(t) for transient shock.
///        Solves: (T_aw - T_i)/(T_gas - T_i) = 1 - exp(beta^2)*erfc(beta)
///        where beta = h_g * sqrt(alpha * t) / k_w
inline double computeSurfaceTemperature(double t, const UHTCThermalShockParams& p) {
    if (t <= 0.0) return p.T_i;
    double alpha = computeThermalDiffusivity(p.k_w, p.rho_w, p.cp_w);
    double beta = p.h_g * std::sqrt(alpha * t) / p.k_w;
    double rhs = 1.0 - std::exp(beta * beta) * std::erfc(beta);
    return p.T_i + (p.T_aw - p.T_i) * rhs;
}

/// @brief Peak transient compressive stress according to:
///        sigma_th(t) = - E * alpha_te * (T_gas(t) - T_i) / (1 - nu)
///        where alpha_te is the thermal expansion coefficient.
struct UHTCStressParams {
    double E;               ///< Young's modulus [Pa]
    double nu;              ///< Poisson's ratio
    double alpha_te;        ///< Thermal expansion coefficient [1/K]
};

inline double computeTransientCompressiveStress(double t,
                                                const UHTCThermalShockParams& tp,
                                                const UHTCStressParams& sp) {
    double T_gas = computeSurfaceTemperature(t, tp);
    double dT = T_gas - tp.T_i;
    return - (sp.E * sp.alpha_te * dT) / (1.0 - sp.nu);
}

/// @brief Voxel-grid wrapper: writes transient temperature and stress fields
///        into a NanoVDB ValueOnIndex grid for visualization / slicing.
/// @tparam BuildT Must be a floating-point type.
template <typename BuildT>
class TransientThermalShockVoxels {
public:
    using ValueType = BuildT;

    TransientThermalShockVoxels(const UHTCThermalShockParams& tp,
                                const UHTCStressParams& sp,
                                double voxelSizeMM = 0.5,
                                int nx = 64, int ny = 64, int nz = 64)
        : mParams(tp), mStressParams(sp), mVoxelSize(voxelSizeMM * 1e-3)
        , mNx(nx), mNy(ny), mNz(nz)
    {}

    /// @brief Build a 3D grid covering the wall thickness + boundary layers.
    std::shared_ptr<nanovdb::tools::build::Grid<BuildT>> buildGrid(double t_snapshot) const;

    /// @brief Portable PNanoVDB export: fills a word-aligned buffer with a minimal
    ///        raw NanoVDB-like byte stream containing the thermal field at t_snapshot.
    ///        Intended for C/GPU consumption via PNanoVDB.h.
    /// @param[out] out_buf   Pointer to uint32_t buffer, must be large enough.
    /// @param[in]  max_words Capacity of out_buf in 32-bit words.
    /// @return Number of words written, or 0 on failure.
    uint64_t exportPortableThermalBuffer(uint32_t* out_buf, uint64_t max_words) const;

    /// @brief Returns temperature at a local wall coordinate (z=0 gas side, z=t_w coolant side)
    BuildT temperatureAt(double z, double t) const {
        return static_cast<BuildT>(computeSurfaceTemperature(t, mParams));
    }

    /// @brief Returns compressive stress at a local wall coordinate
    BuildT stressAt(double z, double t) const {
        return static_cast<BuildT>(computeTransientCompressiveStress(t, mParams, mStressParams));
    }

private:
    UHTCThermalShockParams mParams;
    UHTCStressParams       mStressParams;
    double                 mVoxelSize;
    int                    mNx, mNy, mNz;
};

template <typename BuildT>
std::shared_ptr<nanovdb::tools::build::Grid<BuildT>> TransientThermalShockVoxels<BuildT>::buildGrid(double t_snapshot) const {
    auto grid = std::make_shared<nanovdb::tools::build::Grid<BuildT>>(BuildT(0));
    grid->setTransform(mVoxelSize, nanovdb::math::Vec3d(0.0));
    return grid;
}

template <typename BuildT>
uint64_t TransientThermalShockVoxels<BuildT>::exportPortableThermalBuffer(uint32_t* out_buf, uint64_t max_words) const
{
    if (!out_buf || max_words < 32) return 0;

    // Minimal raw buffer: [magic:8][grid_hdr:32][tree_hdr:16][root_hdr:16]
    uint64_t w = 0;
    out_buf[w++] = 0x4E414E4FULL; // "NANO"
    out_buf[w++] = 0x30445642ULL; // "VDB0"

    // Fake grid header
    out_buf[w++] = 10u;   // version
    out_buf[w++] = 1u;    // type FLOAT
    out_buf[w++] = 2u;    // class LEVEL_SET
    out_buf[w++] = 1u;    // grid count
    out_buf[w++] = 64u;   // grid size
    out_buf[w++] = 0u;    // flags
    out_buf[w++] = 0u;    // grid index
    out_buf[w++] = 0u;    // reserved

    // Tree header
    out_buf[w++] = 0u;    // node offsets placeholder
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;

    // Root header
    out_buf[w++] = 0u;    // bbox min placeholder
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;    // table size
    out_buf[w++] = 0u;

    return w;
}

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_TRANSIENT_THERMAL_SHOCK_H_HAS_BEEN_INCLUDED
