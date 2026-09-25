#include "NozzleProfile.h"

#include <cassert>
#include <cmath>
#include <string>

int main() {
    geometry_engine::NozzleProfileParameters parameters;
    parameters.throat_radius_m = 0.065;
    parameters.exit_radius_m = 0.55;
    parameters.length_m = 3.0;
    parameters.samples = 101;

    std::string error;
    assert(geometry_engine::validate(parameters, error));
    const auto profile = geometry_engine::generateSmoothProfile(parameters);
    assert(profile.size() == parameters.samples);
    assert(std::abs(profile.front().radius_m - parameters.throat_radius_m) < 1e-12);
    assert(std::abs(profile.back().radius_m - parameters.exit_radius_m) < 1e-12);
    for (std::size_t index = 1; index < profile.size(); ++index) {
        assert(profile[index].z_m > profile[index - 1].z_m);
        assert(profile[index].radius_m >= profile[index - 1].radius_m);
    }
    return 0;
}