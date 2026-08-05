#ifndef NANOVDB_TOOLS_SUPERCRITICAL_CHANNEL_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_SUPERCRITICAL_CHANNEL_H_HAS_BEEN_INCLUDED

#include <nanovdb/NanoVDB.h>
#include <cmath>
#include <algorithm>

namespace nanovdb {
namespace tools {

/// @brief Coolant state for supercritical hydrogen / methane channels
struct SupercriticalCoolantState {
    double P;               ///< Pressure [Pa] (> P_crit)
    double T_bulk;          ///< Bulk temperature [K]
    double G;               ///< Mass flux [kg/(m²·s)]
    double q_flux;          ///< Heat flux [W/m²]
    double D_h;             ///< Hydraulic diameter [m]
};

/// @brief Pseudocritical temperature lookup for H2 and LCH4
inline double getPseudocriticalTemperature(const SupercriticalCoolantState& state) {
    // Simplified lookup for H2/LCH4; extend with full NIST correlations
    // H2: T_pc ≈ 33.2 K at 1.3 MPa; scales weakly with pressure
    // For now return a placeholder; caller should set based on real fluid tables
    return 33.2;
}

/// @brief Modified Jackson Nusselt correlation for supercritical flow
///        Nu = 0.021 * Re^0.8 * Pr^0.4 * (rho_b / rho_w)^0.3 * (cp_b / cp_mean)^n
inline double computeSupercriticalNusselt(const SupercriticalCoolantState& state,
                                           double rho_b, double mu_b, double cp_b,
                                           double rho_w, double cp_mean, double k_w) {
    double Re = state.G * state.D_h / mu_b;
    double Pr = mu_b * cp_b / k_w;
    double Nu = 0.021 * std::pow(Re, 0.8) * std::pow(Pr, 0.4)
              * std::pow(rho_b / rho_w, 0.3) * std::pow(cp_b / cp_mean, 1.0);
    return Nu;
}

/// @brief Determines if Heat Transfer Deterioration (HTD) is likely:
///        q_flux / G > q_flux_critical for current (P, T_bulk)
inline bool isHTDRisk(const SupercriticalCoolantState& state, double q_crit_over_G) {
    return (state.q_flux / state.G) > q_crit_over_G;
}

/// @brief Micro-mapped channel width modulation to suppress HTD.
///        Returns local channel width scaled by suppression factor when near T_pc.
inline double computeChannelWidth(const PKVector3& point,
                                  float base_width,
                                  const SupercriticalCoolantState& state,
                                  double T_pseudocritical,
                                  double Delta_T_critical,
                                  float HTD_Suppression_Factor) {
    float T_bulk = static_cast<float>(state.T_bulk);
    if (std::fabs(T_bulk - T_pseudocritical) < Delta_T_critical) {
        return base_width * (1.0f - HTD_Suppression_Factor);
    }
    return base_width;
}

/// @brief Builds an implicit micro-channel SDF inside a solid block.
///        Positive outside solid, negative inside channels.
///        Uses gyroid-like modulation driven by supercritical state.
class MicroMappedChannelSDF {
public:
    MicroMappedChannelSDF(const SupercriticalCoolantState& state,
                          double base_width,
                          double T_pseudocritical,
                          double Delta_T_critical,
                          float suppression_factor)
        : mState(state)
        , mBaseWidth(base_width)
        , mTPc(T_pseudocritical)
        , mDeltaTc(Delta_T_critical)
        , mSuppression(suppression_factor)
    {}

    /// @brief Signed distance-like field for a micro-channel at local point.
    ///        Positive = solid, Negative = channel / coolant.
    float operator()(const PKVector3& point) const {
        float width = computeChannelWidth(point,
                                          static_cast<float>(mBaseWidth),
                                          mState,
                                          mTPc, mDeltaTc, mSuppression);
        // Simple cylindrical channel along Z; replace with gyroid modulation
        float r = std::sqrt(point.X * point.X + point.Y * point.Y);
        return r - width * 0.5f;
    }

private:
    SupercriticalCoolantState mState;
    double                    mBaseWidth;
    double                    mTPc;
    double                    mDeltaTc;
    float                     mSuppression;
};

/// @brief Integrates micro-mapped channels into a solid body via CSG:
///        F_final(p) = max( f_spike(p), -f_channels(p) )
inline float compositeSolidWithChannels(float f_spike,
                                        const MicroMappedChannelSDF& channelSDF,
                                        const PKVector3& p) {
    return std::max(f_spike, -channelSDF(p));
}

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_SUPERCRITICAL_CHANNEL_H_HAS_BEEN_INCLUDED
