#!/usr/bin/env python3
"""Run a deterministic turbopump-to-cold-chamber startup experiment."""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
import sys

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from combustion_internals.cold_chamber import update_pressure
from turbopump_engine.models.cold_flow_model import GRAVITY, friction_factor


def ramp(time_s: float, start_s: float, end_s: float) -> float:
    if time_s <= start_s:
        return 0.0
    if time_s >= end_s:
        return 1.0
    return (time_s - start_s) / (end_s - start_s)


def simulate(case: dict) -> list[dict]:
    dynamic = case["dynamic"]
    shared = case["shared"]
    dt = dynamic["time_step_s"]
    rows = []
    chamber_pressure = 0.0
    flows = {name: 0.0 for name in case["fluids"]}
    estopped = False
    step_count = int(dynamic["duration_s"] / dt) + 1

    for step in range(step_count):
        time_s = step * dt
        requested_valve = ramp(time_s, dynamic["valve_start_s"], dynamic["valve_full_open_s"])
        rpm_fraction = 1.0 - math.exp(-time_s / dynamic["pump_time_constant_s"])
        rpm = shared["shaft_speed_rpm"] * rpm_fraction
        valve = 0.0 if estopped else requested_valve
        inlet_pressures = []
        npsh_values = []
        power_w = 0.0
        total_flow = 0.0
        fluid_values = {}

        for name, fluid in case["fluids"].items():
            nominal_flow = fluid["mass_flow_kg_s"]
            target_flow = nominal_flow * valve * rpm_fraction
            flows[name] += (target_flow - flows[name]) * dt / dynamic["flow_time_constant_s"]
            flow = max(0.0, flows[name])
            diameter = fluid["pipe_diameter_m"]
            area = math.pi * diameter**2 / 4.0
            velocity = flow / (fluid["density_kg_m3"] * area)
            reynolds = fluid["density_kg_m3"] * velocity * diameter / fluid["dynamic_viscosity_pa_s"] if velocity else 0.0
            factor = friction_factor(max(reynolds, 1.0), shared["pipe_roughness_m"], diameter) if velocity else 0.0
            velocity_head = velocity**2 / (2.0 * GRAVITY)
            line_loss = (factor * shared["pipe_length_m"] / diameter + shared["minor_loss_coefficient"]) * velocity_head
            pump_rise = dynamic["rated_pressure_rise_pa"] * rpm_fraction**2 * max(valve, 0.05)
            injector_fraction = flow / nominal_flow if nominal_flow else 0.0
            injector_drop = fluid["injector_pressure_drop_pa"] * injector_fraction**2
            injector_inlet = shared["inlet_pressure_pa_abs"] + pump_rise - line_loss
            chamber_inlet = injector_inlet - injector_drop
            npsh = ((shared["inlet_pressure_pa_abs"] - shared["vapor_pressure_pa_abs"]) /
                    (fluid["density_kg_m3"] * GRAVITY)) + velocity_head
            power = flow * max(pump_rise, 0.0) / fluid["density_kg_m3"] / shared["pump_efficiency"]
            inlet_pressures.append(chamber_inlet)
            npsh_values.append(npsh)
            power_w += power
            total_flow += flow
            fluid_values[name] = {
                "flow_kg_s": flow,
                "pump_pressure_pa": injector_inlet,
                "injector_pressure_drop_pa": injector_drop,
                "npsh_available_m": npsh,
            }

        chamber_pressure = update_pressure(chamber_pressure, inlet_pressures, dt, dynamic["chamber_time_constant_s"])
        alarms = []
        if min(npsh_values, default=0.0) < dynamic["minimum_npsh_margin_m"] and valve > 0.0:
            alarms.append("LOW_NPSH")
        if chamber_pressure > dynamic["maximum_chamber_pressure_pa"]:
            alarms.append("CHAMBER_PRESSURE_HIGH")
        if rpm > dynamic["maximum_rpm"]:
            alarms.append("OVERSPEED")
        if alarms:
            estopped = True

        rows.append({
            "time_s": time_s,
            "requested_valve": requested_valve,
            "valve": valve,
            "rpm": rpm,
            "total_flow_kg_s": total_flow,
            "chamber_pressure_pa": chamber_pressure,
            "pump_power_w": power_w,
            "minimum_npsh_m": min(npsh_values, default=0.0),
            "estopped": estopped,
            "alarms": ";".join(alarms),
            "fluids": fluid_values,
        })
    return rows


def write_outputs(case: dict, output_directory: Path) -> dict:
    rows = simulate(case)
    output_directory.mkdir(parents=True, exist_ok=True)
    telemetry_path = output_directory / "startup_telemetry.csv"
    fields = ["time_s", "requested_valve", "valve", "rpm", "total_flow_kg_s",
              "chamber_pressure_pa", "pump_power_w", "minimum_npsh_m", "estopped", "alarms"]
    with telemetry_path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows({field: row[field] for field in fields} for row in rows)
    summary = {
        "case_id": case["case_id"],
        "telemetry_file": telemetry_path.as_posix(),
        "sample_count": len(rows),
        "final": {key: rows[-1][key] for key in
                  ("time_s", "valve", "rpm", "total_flow_kg_s", "chamber_pressure_pa", "pump_power_w", "minimum_npsh_m", "estopped", "alarms")},
        "alarm_count": sum(bool(row["alarms"]) for row in rows),
    }
    (output_directory / "startup_summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    return summary


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", type=Path)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    case = json.loads(args.case.read_text(encoding="utf-8"))
    print(json.dumps(write_outputs(case, args.output_directory), indent=2))


if __name__ == "__main__":
    main()