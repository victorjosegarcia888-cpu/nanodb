# Conceptual analysis architecture

```text
case parameters
      |
      v
PicoGK/C# + NanoVDB/OpenVDB  -->  SDF / embedded-boundary geometry
      |                                      |
      v                                      v
combustion_internals C++/CUDA       AMReX transport and AMR
      |                                      |
      +---------- fields and diagnostics ---+
                         |
                         v
                 UHTC / woven screening
                         |
                         v
                 PicoGK viewer / GLTF export
```

## Ownership boundaries

- `geometry_engine`: host-side profiles, PicoGK data bridge and voxel/SDF boundaries.
- `rocket_advanced`: case contract and CUDA geometry kernels.
- `combustion_internals`: flow, chemistry and diagnostics.
- `uhtc`: thermal and structural screening.
- `woven`: topology and lattice generation.
- `rust`: file, voxel and safety-oriented utilities where ownership and bounds checking help.
- `picogk`, `PicoGKRuntime`, `nanodb`, `openvdb`, `Src`: upstream or foundational libraries; do not fork their APIs for a first integration.

## Data contract

All exchanged fields need a case id, SI units, grid origin, spacing, dimensions, time, source model and validation status. GPU kernels should reject invalid dimensions and report CUDA errors after launch and synchronization.