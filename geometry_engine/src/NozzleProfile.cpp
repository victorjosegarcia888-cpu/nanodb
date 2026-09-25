#include "NozzleProfile.h"

#include <cmath>
#include <fstream>

namespace geometry_engine {

bool validate(const NozzleProfileParameters& parameters, std::string& error) {
    if (!std::isfinite(parameters.throat_radius_m) || parameters.throat_radius_m <= 0.0 ||
        !std::isfinite(parameters.exit_radius_m) || parameters.exit_radius_m <= parameters.throat_radius_m ||
        !std::isfinite(parameters.length_m) || parameters.length_m <= 0.0 ||
        parameters.samples < 2) {
        error = "profile dimensions must be finite, positive and expanding";
        return false;
    }
    return true;
}

std::vector<ProfilePoint> generateSmoothProfile(const NozzleProfileParameters& parameters) {
    std::string error;
    if (!validate(parameters, error)) {
        return {};
    }

    std::vector<ProfilePoint> profile;
    profile.reserve(parameters.samples);
    for (std::size_t index = 0; index < parameters.samples; ++index) {
        const double t = static_cast<double>(index) / static_cast<double>(parameters.samples - 1);
        const double smooth = 10.0 * std::pow(t, 3.0) - 15.0 * std::pow(t, 4.0) + 6.0 * std::pow(t, 5.0);
        profile.push_back({parameters.length_m * t,
                          parameters.throat_radius_m +
                              (parameters.exit_radius_m - parameters.throat_radius_m) * smooth});
    }
    return profile;
}

bool writeProfileCsv(const std::string& path, const std::vector<ProfilePoint>& profile) {
    std::ofstream output(path);
    if (!output) {
        return false;
    }
    output << "z_m,radius_m\n";
    for (const ProfilePoint& point : profile) {
        output << point.z_m << ',' << point.radius_m << '\n';
    }
    return static_cast<bool>(output);
}

} // namespace geometry_engine