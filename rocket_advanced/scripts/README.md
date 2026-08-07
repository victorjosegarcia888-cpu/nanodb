# Advanced Rocket Geometries — Second Code Path

Esta carpeta contiene el flujo avanzado para geometrías de motores de cohetes difíciles de fabricar, inspirado en PicoGK/PicoGKRuntime pero implementado sobre nanodb + CUDA.

## Filosofía

- **Camino 1 (actual)**: modelos analíticos, kernels CUDA básicos, slicer, toolpaths
- **Camino 2 (esta carpeta)**: geometrías regenerativas avanzadas, TPMS, lattices, metadatos de fabricación, visualización

## Ideas tomadas de PicoGK/PicoGKRuntime

1. `Voxels::RenderImplicit` → `cuda::renderImplicitSDF` (voxelización de SDF arbitrario)
2. `Lattice::AddBeam/AddSphere` → `cuda::renderLatticeBeams` (vigas lattice en CUDA)
3. `Voxels::BoolAdd/Subtract/Intersect` → `cuda::csgOperation` (operaciones CSG en device)
4. `Voxels::Offset` → `cuda::offsetField` (shells de espesor variable)
5. `ScalarField/VectorField` → `cuda::ScalarField/VectorField` (campos térmicos/de flujo)
6. `VdbMeta` → metadatos embebidos en buffers portable NanoVDB
7. `Viewer_EnableGroupWarnOverhang` → detección de overhangs en CUDA para soportes AM
8. `Voxels::roAsMesh()` → `cuda::extractSurfaceMesh` (exportación STL/CLI desde CUDA)

## Estructura propuesta

```
rocket_advanced/
├── include/
│   └── AdvancedRocketGeometries.h   # API C++/CUDA avanzada
├── src/
│   ├── AdvancedRocketGeometries.cu  # Kernels CUDA
│   └── AdvancedRocketGeometries.cpp # Host wrappers
├── tests/
│   └── test_advanced_rocket.cpp     # Tests avanzados
└── scripts/
    └── README.md                    # Esta documentación
```

## Casos de uso objetivo

- Toberas aerospike con canales de refrigeración TPMS/gyroid
- Cara de inyector con lattice variable y orificios impresos
- Camisas regenerativas con espesor variable y trayectos de flujo optimizados
- Metadatos de material/impresora embebidos en grilla NanoVDB portable
- Visualización integrada con PicoGK viewer

## Próximos pasos sugeridos

1. Implementar `renderImplicitSDF` genérico en CUDA
2. Portar `Lattice::AddBeam` a `renderLatticeBeams` CUDA
3. Implementar CSG booleano en device: `csgUnion`, `csgSubtract`, `csgIntersect`
4. Agregar metadatos a `exportPortableThermalBuffer` y similares
5. Conectar `AdvancedRocketGeometries` con `RocketToolpathGenerator`
