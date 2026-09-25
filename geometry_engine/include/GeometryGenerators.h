#pragma once

#include "NozzleProfile.h"

#include <cstddef>
#include <string>
#include <vector>

namespace geometry_engine {

struct ChamberParameters {
    double chamber_radius_m = 0.0;
    double chamber_length_m = 0.0;
    double throat_radius_m = 0.0;
    double nozzle_length_m = 0.0;
    double exit_radius_m = 0.0;
    std::size_t samples = 0;
};

struct AerospikeParameters {
    double throat_radius_m = 0.0;
    double base_radius_m = 0.0;
    double spike_length_m = 0.0;
    std::size_t samples = 0;
};

struct InjectorParameters {
    double face_radius_m = 0.0;
    double hole_radius_m = 0.0;
    double ring_pitch_m = 0.0;
    std::size_t ring_count = 0;
    std::size_t holes_per_ring = 0;
};

struct InjectorHole {
    double x_m = 0.0;
    double y_m = 0.0;
    double radius_m = 0.0;
    std::size_t ring = 0;
};

struct CoolingChannelParameters {
    double inner_radius_m = 0.0;
    double outer_radius_m = 0.0;
    double channel_radius_m = 0.0;
    double axial_length_m = 0.0;
    std::size_t channel_count = 0;
    std::size_t samples_per_channel = 0;
};

struct CoolingChannelPoint {
    double x_m = 0.0;
    double y_m = 0.0;
    double z_m = 0.0;
};

bool validate(const ChamberParameters& parameters, std::string& error);
bool validate(const AerospikeParameters& parameters, std::string& error);
bool validate(const InjectorParameters& parameters, std::string& error);
bool validate(const CoolingChannelParameters& parameters, std::string& error);

std::vector<ProfilePoint> generateChamberProfile(const ChamberParameters& parameters);
std::vector<ProfilePoint> generateAerospikeProfile(const AerospikeParameters& parameters);
std::vector<InjectorHole> generateInjectorPattern(const InjectorParameters& parameters);
std::vector<std::vector<CoolingChannelPoint>> generateCoolingChannels(
    const CoolingChannelParameters& parameters);

} // namespace geometry_engine