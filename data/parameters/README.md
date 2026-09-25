# Design parameters and notes

This directory contains the `.txt` notes that were previously scattered across the repository. Provenance is preserved in two source directories so every table can be traced back to its original location.

## Content map

| Grupo | Contenido principal |
| --- | --- |
| `directories-work/` | general parameters, tables, structure and organization notes |
| `table-important/` | LOX/CH4, propellants, engines, turbines, bearings, structures and test conditions |

The `table-important/` source groups `combustion-chamber/`, `fuel-tanks/`, `other-rocket/` and `other-books/` preserve notes that were nested in the original source.

## Detected variable families

- **Propellants and thermochemistry:** LOX, CH4, mixture ratios, propellant properties, temperature and pressure.
- **Chamber and nozzle:** throat, chamber, mass flow, expansion, Mach and geometry.
- **Feed systems and turbopumps:** flow rate, pressure, turbines, bearings and operating conditions.
- **Structures and materials:** loads, structure, UHTC, cooling and manufacturing.
- **Simulation:** AMReX, NanoVDB, voxelization, CUDA and validation conditions.

## Recommended workflow

1. Identify the variable and its unit in the original note.
2. Record the exact source and separate published data, assumptions and calculated results.
3. Convert data to a structured format before connecting it to C++, CUDA, C# or Rust.
4. Validate dimensions, ranges and sensitivity before feeding a simulation.

Values were not normalized or reinterpreted during the repository reorganization. These notes are not, by themselves, a manufacturing, ignition, test or flight specification.