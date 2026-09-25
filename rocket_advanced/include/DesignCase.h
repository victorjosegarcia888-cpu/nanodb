#pragma once

#include <cmath>
#include <string>

namespace rocket_advanced {

struct ThermodynamicInputs {
    double chamber_pressure_pa = 0.0;
    double combustion_temperature_k = 0.0;
    double mixture_ratio_oxidizer_to_fuel = 0.0;
    double characteristic_velocity_m_per_s = 0.0;
    double wall_heat_flux_w_per_m2 = 0.0;
};

struct GeometryInputs {
    int chamber_count = 0;
    double chamber_diameter_m = 0.0;
    double chamber_length_m = 0.0;
    double throat_diameter_m = 0.0;
    double exit_diameter_m = 0.0;
    double nozzle_length_m = 0.0;
    double expansion_ratio = 0.0;
};

struct TurbomachineryInputs {
    double shaft_speed_rpm = 0.0;
    double lox_mass_flow_kg_per_s = 0.0;
    double methane_mass_flow_kg_per_s = 0.0;
    double pump_power_w = 0.0;
};

struct DesignCase {
    std::string case_id;
    std::string status = "reference_only";
    ThermodynamicInputs thermodynamics;
    GeometryInputs geometry;
    TurbomachineryInputs turbomachinery;
};

inline bool validateDesignCase(const DesignCase& design_case, std::string& error) {
    const auto& thermo = design_case.thermodynamics;
    const auto& geometry = design_case.geometry;
    const auto& pumps = design_case.turbomachinery;

    if (design_case.case_id.empty()) {
        error = "case_id must not be empty";
        return false;
    }
    if (design_case.status != "reference_only") {
        error = "only reference_only cases are accepted by the conceptual validator";
        return false;
    }
    if (!std::isfinite(thermo.chamber_pressure_pa) || thermo.chamber_pressure_pa <= 0.0 ||
        !std::isfinite(thermo.combustion_temperature_k) || thermo.combustion_temperature_k <= 0.0 ||
        !std::isfinite(thermo.mixture_ratio_oxidizer_to_fuel) || thermo.mixture_ratio_oxidizer_to_fuel <= 0.0 ||
        !std::isfinite(thermo.characteristic_velocity_m_per_s) || thermo.characteristic_velocity_m_per_s <= 0.0) {
        error = "thermodynamic inputs must be finite and positive";
        return false;
    }
    if (geometry.chamber_count <= 0 || geometry.chamber_diameter_m <= 0.0 ||
        geometry.chamber_length_m <= 0.0 || geometry.throat_diameter_m <= 0.0 ||
        geometry.exit_diameter_m <= geometry.throat_diameter_m || geometry.nozzle_length_m <= 0.0 ||
        geometry.expansion_ratio <= 1.0) {
        error = "geometry inputs are inconsistent or non-positive";
        return false;
    }
    if (!std::isfinite(pumps.shaft_speed_rpm) || pumps.shaft_speed_rpm <= 0.0 ||
        !std::isfinite(pumps.lox_mass_flow_kg_per_s) || pumps.lox_mass_flow_kg_per_s <= 0.0 ||
        !std::isfinite(pumps.methane_mass_flow_kg_per_s) || pumps.methane_mass_flow_kg_per_s <= 0.0 ||
        !std::isfinite(pumps.pump_power_w) || pumps.pump_power_w <= 0.0) {
        error = "turbomachinery inputs must be finite and positive";
        return false;
    }
    return true;
}

} // namespace rocket_advanced