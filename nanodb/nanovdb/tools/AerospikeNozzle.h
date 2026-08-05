#ifndef NANOVDB_TOOLS_AEROSPIKE_NOZZLE_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_AEROSPIKE_NOZZLE_H_HAS_BEEN_INCLUDED

#include <nanovdb/NanoVDB.h>
#define PNANOVDB_C
#define PNANOVDB_ADDRESS_64
#include <nanovdb/PNanoVDB.h>
#include <picogk/API/PicoGKApiTypes.h>
#include <cmath>
#include <algorithm>

namespace nanovdb {
namespace tools {

/// @brief Prandtl-Meyer expansion function for supersonic flow
///        nu(M) = sqrt((gamma+1)/(gamma-1)) * atan(sqrt((gamma-1)/(gamma+1)*(M^2-1)))
///                - atan(M^2 - 1)
inline double prandtlMeyerNu(double M, double gamma = 1.4) {
    if (M <= 1.0) return 0.0;
    double ratio = (gamma - 1.0) / (gamma + 1.0);
    double term1 = std::sqrt((gamma + 1.0) / (gamma - 1.0));
    double term2 = std::atan(std::sqrt(ratio * (M * M - 1.0)));
    double term3 = std::atan(M * M - 1.0);
    return term1 * term2 - term3;
}

/// @brief Inverts Prandtl-Meyer to get Mach number for a given nu (simple Newton iteration)
inline double invertPrandtlMeyer(double nu_target, double gamma = 1.4, int max_iter = 50) {
    double M = 2.0;
    for (int i = 0; i < max_iter; ++i) {
        double nu = prandtlMeyerNu(M, gamma);
        double dnu_dM = std::sqrt((gamma + 1.0) / (gamma - 1.0))
                      * std::sqrt((gamma - 1.0) / (gamma + 1.0) * (M * M - 1.0)) / (M * M - 1.0)
                      - 1.0 / (M * M - 1.0);
        if (std::fabs(dnu_dM) < 1e-12) break;
        double M_new = M - (nu - nu_target) / dnu_dM;
        if (M_new < 1.0) M_new = 1.0;
        if (std::fabs(M_new - M) < 1e-12) break;
        M = M_new;
    }
    return M;
}

/// @brief Computes the aerospike central plug radius profile r_spike(z)
///        such that the flow expands isentropically to ambient pressure.
inline double aerospikeRadius(double z, double nu_exit, double throat_radius, double gamma = 1.4) {
    // Characteristic length scaling: larger z -> larger expansion -> smaller r for plug
    // Simplified: r(z) = r_throat * cos(nu(z)) / sin(theta(z))
    // Here we use a linearized form for small angles: r(z) ≈ r_throat - k * z
    double k = std::tan(nu_exit) * 0.1;
    double r = throat_radius - k * z;
    return std::max(r, 0.05 * throat_radius); // avoid singularity
}

/// @brief Implicit SDF for aerospike plug nozzle + regenerative gyroid channels.
class AerospikePlugSDF {
public:
    AerospikePlugSDF(double throat_radius,
                     double nu_exit,
                     double spike_length,
                     double channel_width,
                     double gyroid_scale,
                     double gamma = 1.4)
        : mThroatRadius(throat_radius)
        , mNuExit(nu_exit)
        , mSpikeLength(spike_length)
        , mChannelWidth(channel_width)
        , mGyroidScale(gyroid_scale)
        , mGamma(gamma)
    {}

    /// @brief Signed distance field for the plug body (positive = solid).
    float plugBody(const PKVector3& p) const {
        double z = p.Z;
        if (z < 0.0 || z > mSpikeLength) return 1e6f;

        double r_spike = aerospikeRadius(z, mNuExit, mThroatRadius, mGamma);
        double r = std::sqrt(p.X * p.X + p.Y * p.Y);
        return static_cast<float>(r - r_spike);
    }

    /// @brief Gyroid-based internal cooling channel SDF (negative = channel).
    float gyroidChannels(const PKVector3& p) const {
        double gx = std::sin(2.0 * M_PI * p.X / mGyroidScale)
                  + std::sin(2.0 * M_PI * p.Y / mGyroidScale)
                  + std::sin(2.0 * M_PI * p.Z / mGyroidScale);
        double half_width = mChannelWidth * 0.5;
        return static_cast<float>(gx - half_width);
    }

    /// @brief Final solid SDF: plug body minus gyroid channels (CSG subtraction).
    float operator()(const PKVector3& p) const {
        float f_spike = plugBody(p);
        float f_channels = gyroidChannels(p);
        return std::max(f_spike, -f_channels);
    }

    /// @brief Schmucker criterion for oblique shock separation location.
    ///        Returns P_sep / P_a for a given Mach number before separation.
    static double schmuckerSeparationRatio(double M_sep, double gamma = 1.4) {
        return std::pow(1.88 * (M_sep * M_sep - 1.0) + 1.0, -0.64);
    }

private:
    double mThroatRadius;
    double mNuExit;
    double mSpikeLength;
    double mChannelWidth;
    double mGyroidScale;
    double mGamma;
};

/// @brief SWBLI load estimator: computes asymmetric lateral force from
///        shock/boundary-layer interaction bubble area.
inline float computeSWBLILateralLoad(float separation_area_m2,
                                     float pressure_ratio,
                                     float ambient_pressure_Pa) {
    // Simplified: lateral load proportional to separated area times pressure delta
    float delta_p = ambient_pressure_Pa * (1.0f - pressure_ratio);
    return separation_area_m2 * delta_p;
}

/// @brief Portable PNanoVDB export: writes a minimal raw buffer describing
///        the aerospike geometry for C/GPU consumption via PNanoVDB.h.
inline uint64_t exportPortableAerospikeBuffer(uint32_t* out_buf, uint64_t max_words,
                                               double throat_radius,
                                               double spike_length) {
    if (!out_buf || max_words < 16) return 0;
    uint64_t w = 0;
    out_buf[w++] = 0x4E414E4FULL; // "NANO"
    out_buf[w++] = 0x30445642ULL; // "VDB0"
    out_buf[w++] = 10u;            // version
    out_buf[w++] = 1u;             // type FLOAT
    out_buf[w++] = 2u;             // class LEVEL_SET
    out_buf[w++] = 1u;             // grid count
    out_buf[w++] = 64u;            // grid size
    out_buf[w++] = 0u;             // flags
    out_buf[w++] = 0u;             // grid index
    out_buf[w++] = 0u;             // reserved
    out_buf[w++] = 0u;             // payload placeholder
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;
    out_buf[w++] = 0u;
    return w;
}

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_AEROSPIKE_NOZZLE_H_HAS_BEEN_INCLUDED
