# Geometry engine

This is the project-owned geometry layer described by `structure.txt`. It
does not duplicate PicoGKRuntime, NanoVDB or OpenVDB. It creates validated
parameterized geometry and exposes stable outputs for those libraries.

## Generators

- **Chamber and nozzle:** `generateChamberProfile()` returns the cylindrical
  chamber, throat and smooth expanding nozzle profile.
- **Aerospike:** `generateAerospikeProfile()` creates the conceptual spike
  profile. `rocket_advanced::makeAerospikePlug()` maps it to the existing CUDA
  SDF renderer.
- **Injector:** `generateInjectorPattern()` creates concentric hole centers;
  `makeLatticeInjectorFace()` maps dimensions to CUDA lattice parameters.
- **Cooling:** `generateCoolingChannels()` creates helical channel centerlines;
  `makeRegenerativeJacket()` maps the jacket dimensions to the CUDA gyroid
  renderer.

The host generators are deliberately deterministic and testable without a GPU.
The CUDA layer then voxelizes the selected geometry and can apply CSG,
overhang, thermal-gradient and mesh-export operations.

## Build host-only geometry

```bash
cmake -S geometry_engine -B /tmp/geometry-build
cmake --build /tmp/geometry-build
ctest --test-dir /tmp/geometry-build --output-on-failure
```

The root option `BUILD_GEOMETRY_ENGINE` is optional. The complete
`rocket_advanced` integration additionally requires CUDA and `nvcc`.