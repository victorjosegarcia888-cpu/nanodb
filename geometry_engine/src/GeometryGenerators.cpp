#include "GeometryGenerators.h"

#include <cmath>

namespace geometry_engine {
namespace {

constexpr double Pi = 3.14159265358979323846;

double smoothStep(double t) {
    return 10.0 * std::pow(t, 3.0) - 15.0 * std::pow(t, 4.0) + 6.0 * std::pow(t, 5.0);
}

bool finitePositive(double value) {
    return std::isfinite(value) && value > 0.0;
}

} // namespace

bool validate(const ChamberParameters& parameters, std::string& error) {
    if (!finitePositive(parameters.chamber_radius_m) ||
        !finitePositive(parameters.chamber_length_m) ||
        !finitePositive(parameters.throat_radius_m) ||
        !finitePositive(parameters.nozzle_length_m) ||
        !finitePositive(parameters.exit_radius_m) ||
        parameters.throat_radius_m >= parameters.chamber_radius_m ||
        parameters.exit_radius_m <= parameters.throat_radius_m || parameters.samples < 3) {
        error = "chamber dimensions are invalid or not expanding";
        return false;
    }
    return true;
}

bool validate(const AerospikeParameters& parameters, std::string& error) {
    if (!finitePositive(parameters.throat_radius_m) ||
        !finitePositive(parameters.base_radius_m) ||
        !finitePositive(parameters.spike_length_m) ||
        parameters.base_radius_m <= parameters.throat_radius_m || parameters.samples < 2) {
        error = "aerospike dimensions are invalid";
        return false;
    }
    return true;
}

bool validate(const InjectorParameters& parameters, std::string& error) {
    if (!finitePositive(parameters.face_radius_m) || !finitePositive(parameters.hole_radius_m) ||
        !finitePositive(parameters.ring_pitch_m) || parameters.ring_count == 0 ||
        parameters.holes_per_ring < 3 ||
        parameters.ring_pitch_m * static_cast<double>(parameters.ring_count) >= parameters.face_radius_m ||
        parameters.hole_radius_m >= parameters.ring_pitch_m * 0.5) {
        error = "injector pattern dimensions or counts are invalid";
        return false;
    }
    return true;
}

bool validate(const CoolingChannelParameters& parameters, std::string& error) {
    if (!finitePositive(parameters.inner_radius_m) ||
        !finitePositive(parameters.outer_radius_m) ||
        !finitePositive(parameters.channel_radius_m) ||
        !finitePositive(parameters.axial_length_m) || parameters.outer_radius_m <= parameters.inner_radius_m ||
        parameters.channel_radius_m >= (parameters.outer_radius_m - parameters.inner_radius_m) * 0.5 ||
        parameters.channel_count == 0 || parameters.samples_per_channel < 2) {
        error = "cooling channel dimensions or counts are invalid";
        return false;
    }
    return true;
}

std::vector<ProfilePoint> generateChamberProfile(const ChamberParameters& parameters) {
    std::string error;
    if (!validate(parameters, error)) {
        return {};
    }

    const std::size_t chamber_samples = parameters.samples / 3;
    const std::size_t nozzle_samples = parameters.samples - chamber_samples + 1;
    std::vector<ProfilePoint> profile;
    profile.reserve(parameters.samples + 1);

    for (std::size_t index = 0; index < chamber_samples; ++index) {
        const double t = static_cast<double>(index) / static_cast<double>(chamber_samples - 1);
        profile.push_back({parameters.chamber_length_m * t, parameters.chamber_radius_m});
    }
    profile.push_back({parameters.chamber_length_m, parameters.throat_radius_m});
    for (std::size_t index = 1; index < nozzle_samples; ++index) {
        const double t = static_cast<double>(index) / static_cast<double>(nozzle_samples - 1);
        profile.push_back({parameters.chamber_length_m + parameters.nozzle_length_m * t,
                           parameters.throat_radius_m +
                               (parameters.exit_radius_m - parameters.throat_radius_m) * smoothStep(t)});
    }
    return profile;
}

std::vector<ProfilePoint> generateAerospikeProfile(const AerospikeParameters& parameters) {
    std::string error;
    if (!validate(parameters, error)) {
        return {};
    }

    std::vector<ProfilePoint> profile;
    profile.reserve(parameters.samples);
    for (std::size_t index = 0; index < parameters.samples; ++index) {
        const double t = static_cast<double>(index) / static_cast<double>(parameters.samples - 1);
        profile.push_back({parameters.spike_length_m * t,
                           parameters.base_radius_m -
                               (parameters.base_radius_m - parameters.throat_radius_m) * smoothStep(t)});
    }
    return profile;
}

std::vector<InjectorHole> generateInjectorPattern(const InjectorParameters& parameters) {
    std::string error;
    if (!validate(parameters, error)) {
        return {};
    }

    std::vector<InjectorHole> holes;
    holes.reserve(parameters.ring_count * parameters.holes_per_ring);
    for (std::size_t ring = 0; ring < parameters.ring_count; ++ring) {
        const double radius = parameters.ring_pitch_m * static_cast<double>(ring + 1);
        for (std::size_t hole = 0; hole < parameters.holes_per_ring; ++hole) {
            const double angle = 2.0 * Pi * static_cast<double>(hole) /
                                 static_cast<double>(parameters.holes_per_ring);
            holes.push_back({radius * std::cos(angle), radius * std::sin(angle),
                             parameters.hole_radius_m, ring});
        }
    }
    return holes;
}

std::vector<std::vector<CoolingChannelPoint>> generateCoolingChannels(
    const CoolingChannelParameters& parameters) {
    std::string error;
    if (!validate(parameters, error)) {
        return {};
    }

    std::vector<std::vector<CoolingChannelPoint>> channels;
    channels.resize(parameters.channel_count);
    for (std::size_t channel = 0; channel < parameters.channel_count; ++channel) {
        auto& points = channels[channel];
        points.reserve(parameters.samples_per_channel);
        const double phase = 2.0 * Pi * static_cast<double>(channel) /
                             static_cast<double>(parameters.channel_count);
        const double radius = parameters.inner_radius_m + parameters.channel_radius_m;
        for (std::size_t sample = 0; sample < parameters.samples_per_channel; ++sample) {
            const double t = static_cast<double>(sample) /
                             static_cast<double>(parameters.samples_per_channel - 1);
            const double angle = phase + 2.0 * Pi * t;
            points.push_back({radius * std::cos(angle), radius * std::sin(angle),
                              parameters.axial_length_m * t});
        }
    }
    return channels;
}

} // namespace geometry_engine