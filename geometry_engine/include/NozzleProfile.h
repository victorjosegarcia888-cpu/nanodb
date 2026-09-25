#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace geometry_engine {

struct NozzleProfileParameters {
    double throat_radius_m = 0.0;
    double exit_radius_m = 0.0;
    double length_m = 0.0;
    std::size_t samples = 0;
};

struct ProfilePoint {
    double z_m = 0.0;
    double radius_m = 0.0;
};

bool validate(const NozzleProfileParameters& parameters, std::string& error);

// Generates a smooth conceptual profile using the quintic smoothstep listed
// in the repository notes. It is a geometry seed, not a certified Rao contour.
std::vector<ProfilePoint> generateSmoothProfile(const NozzleProfileParameters& parameters);

bool writeProfileCsv(const std::string& path, const std::vector<ProfilePoint>& profile);

} // namespace geometry_engine