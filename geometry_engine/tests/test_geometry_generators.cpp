#include "GeometryGenerators.h"

#include <cassert>
#include <cmath>
#include <string>

int main() {
    std::string error;

    geometry_engine::ChamberParameters chamber{0.19, 1.1, 0.065, 3.0, 0.55, 100};
    const auto chamber_profile = geometry_engine::generateChamberProfile(chamber);
    assert(geometry_engine::validate(chamber, error));
    assert(chamber_profile.size() > chamber.samples);
    assert(std::abs(chamber_profile.front().radius_m - chamber.chamber_radius_m) < 1e-12);
    assert(std::abs(chamber_profile.back().radius_m - chamber.exit_radius_m) < 1e-12);

    geometry_engine::AerospikeParameters aerospike{0.065, 0.55, 1.5, 64};
    const auto aerospike_profile = geometry_engine::generateAerospikeProfile(aerospike);
    assert(aerospike_profile.size() == aerospike.samples);
    assert(aerospike_profile.front().radius_m > aerospike_profile.back().radius_m);

    geometry_engine::InjectorParameters injector{0.10, 0.002, 0.01, 4, 24};
    const auto holes = geometry_engine::generateInjectorPattern(injector);
    assert(holes.size() == injector.ring_count * injector.holes_per_ring);
    assert(std::abs(holes.front().x_m - injector.ring_pitch_m) < 1e-12);

    geometry_engine::CoolingChannelParameters cooling{0.065, 0.10, 0.005, 1.5, 32, 33};
    const auto channels = geometry_engine::generateCoolingChannels(cooling);
    assert(channels.size() == cooling.channel_count);
    assert(channels.front().size() == cooling.samples_per_channel);
    assert(std::abs(channels.front().front().z_m) < 1e-12);
    assert(std::abs(channels.front().back().z_m - cooling.axial_length_m) < 1e-12);
    return 0;
}