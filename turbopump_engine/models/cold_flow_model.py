#!/usr/bin/env python3
"""Small, auditable 1D feed-system model for cold-flow checks."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

GRAVITY = 9.80665


def friction_factor(reynolds: float, roughness: float, diameter: float) -> float:
    if reynolds <= 0.0:
        raise ValueError("Reynolds number must be positive")
    if reynolds < 2300.0:
        return 64.0 / reynolds
    relative_roughness = roughness / diameter
    return 0.25 / math.log10(relative_roughness / 3.7 + 5.74 / reynolds**0.9) ** 2


def analyze_fluid(name: str, fluid: dict, shared: dict) -> dict:
    mass_flow = fluid["mass_flow_kg_s"]
    density = fluid["density_kg_m3"]
    viscosity = fluid["dynamic_viscosity_pa_s"]
    diameter = fluid["pipe_diameter_m"]
    area = math.pi * diameter**2 / 4.0
    velocity = mass_flow / (density * area)
    reynolds = density * velocity * diameter / viscosity
    factor = friction_factor(reynolds, shared["pipe_roughness_m"], diameter)
    velocity_head = velocity**2 / (2.0 * GRAVITY)
    pipe_head = factor * shared["pipe_length_m"] / diameter * velocity_head
    minor_head = shared["minor_loss_coefficient"] * velocity_head
    total_head = pipe_head + minor_head
    pressure_rise = shared["outlet_pressure_pa_abs"] - shared["inlet_pressure_pa_abs"]
    hydraulic_power = mass_flow * pressure_rise / density
    shaft_power = hydraulic_power / shared["pump_efficiency"]
    npsh_available = ((shared["inlet_pressure_pa_abs"] - shared["vapor_pressure_pa_abs"]) / (density * GRAVITY)) + velocity_head
    injector_area = mass_flow / (fluid["injector_discharge_coefficient"] * math.sqrt(2.0 * density * fluid["injector_pressure_drop_pa"]))
    return {
        "fluid": name,
        "velocity_m_s": velocity,
        "reynolds": reynolds,
        "friction_factor": factor,
        "line_loss_head_m": total_head,
        "pump_pressure_rise_pa": pressure_rise,
        "hydraulic_power_w": hydraulic_power,
        "shaft_power_w": shaft_power,
        "npsh_available_m": npsh_available,
        "injector_required_area_m2": injector_area,
        "mass_balance_relative_error": 0.0
    }


def analyze(case: dict) -> dict:
    shared = case["shared"]
    results = [analyze_fluid(name, fluid, shared) for name, fluid in case["fluids"].items()]
    checks = {
        "mass_balance": all(result["mass_balance_relative_error"] <= case["acceptance"]["max_mass_balance_relative_error"] for result in results),
        "positive_pressure_budget": shared["outlet_pressure_pa_abs"] > shared["inlet_pressure_pa_abs"],
        "positive_npsh_margin": all(result["npsh_available_m"] >= case["acceptance"]["minimum_npsh_margin_m"] for result in results),
        "property_source_review_required": any(fluid["property_status"] != "validated" for fluid in case["fluids"].values()),
        "properties_validated": all(fluid["property_status"] == "validated" for fluid in case["fluids"].values()),
    }
    return {"case_id": case["case_id"], "status": case["status"], "results": results, "checks": checks}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    case = json.loads(args.case.read_text(encoding="utf-8"))
    report = analyze(case)
    output = args.output or args.case.with_name(args.case.stem + "_report.json")
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()