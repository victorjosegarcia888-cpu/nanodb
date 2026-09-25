#include <cassert>
#include <string>

#include "DesignCase.h"

int main() {
    rocket_advanced::DesignCase design_case;
    design_case.case_id = "lox_ch4_concept_reference";
    design_case.thermodynamics.chamber_pressure_pa = 30.0e6;
    design_case.thermodynamics.combustion_temperature_k = 3500.0;
    design_case.thermodynamics.mixture_ratio_oxidizer_to_fuel = 3.6;
    design_case.thermodynamics.characteristic_velocity_m_per_s = 1700.0;
    design_case.geometry.chamber_count = 4;
    design_case.geometry.chamber_diameter_m = 0.38;
    design_case.geometry.chamber_length_m = 1.1;
    design_case.geometry.throat_diameter_m = 0.13;
    design_case.geometry.exit_diameter_m = 1.1;
    design_case.geometry.nozzle_length_m = 3.0;
    design_case.geometry.expansion_ratio = 90.0;
    design_case.turbomachinery.shaft_speed_rpm = 28000.0;
    design_case.turbomachinery.lox_mass_flow_kg_per_s = 900.0;
    design_case.turbomachinery.methane_mass_flow_kg_per_s = 380.0;
    design_case.turbomachinery.pump_power_w = 140.0e6;

    std::string error;
    assert(rocket_advanced::validateDesignCase(design_case, error));

    design_case.geometry.exit_diameter_m = 0.10;
    assert(!rocket_advanced::validateDesignCase(design_case, error));
    return 0;
}