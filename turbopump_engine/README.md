# Turbopump and feed-system laboratory

This module starts with a transparent 1D cold-flow model. It is intended to
check units, mass conservation, pressure budget, hydraulic power and NPSH
before any AMReX multiphase or CUDA model is attempted.

## Layout

- `cases/`: reproducible input cases and generated reports.
- `models/`: small reference models with no external runtime dependency.
- `tests/`: regression tests for conservation and limiting cases.
- `inducer/`: future inducer geometry and cavitation studies.
- `bearings/`: hydrostatic bearing data and rotor-dynamic screening.
- `cryogenic/`: property providers with explicit source and temperature range.

The first case is a scaled laboratory case, not a Raptor specification. Its
properties are estimates that must be replaced by validated cryogenic tables.

Run it with:

```bash
python3 -m unittest discover -s turbopump_engine/tests -v
python3 turbopump_engine/models/cold_flow_model.py \
	turbopump_engine/cases/lox_ch4_cold_flow_scaled.json \
	--output turbopump_engine/cases/lox_ch4_cold_flow_scaled_report.json
```

The report intentionally distinguishes positive numerical checks from
`properties_validated: false`. That flag must remain false until the density,
viscosity and vapor-pressure sources are reviewed for the actual temperature
and pressure range.