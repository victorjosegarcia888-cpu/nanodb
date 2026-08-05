# Comandos UHTC-NanoDB

## Build completo

```bash
cd /workspaces/uhtc-nanodb-
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## Tests

```bash
cd /workspaces/uhtc-nanodb-/build
ctest --output-on-failure
```

Tests incluidos:
- `picogk_api_test` — tipos PicoGK
- `voxel_basic_test` — headers NanoVDB/nanodb/CUDA
- `uhtc_voxel_analysis_test` — térmico, fluido supercrítico, aerospike
- `voxel_cuda_test` — compilación CUDA + runtime sin GPU
- `voxel_sdf_cuda_test` — kernel SDF + conteo de materiales en GPU

## Análisis cuantitativo Python

```bash
python3 /workspaces/uhtc-nanodb-/analysis/uhtc_analysis.py
```

Salida esperada:
- `delta_th(0.5s) ≈ 7.3 mm`
- `T_gas(500 ms) ≈ 2770 K`
- `sigma_th(500 ms) ≈ -6294 MPa`
- `Nu ≈ 90`, `h_c ≈ 8990 W/(m²·K)`
- `P_sep/P_a ≈ 0.23` (Schmucker)
- `SWBLI lateral load ≈ 7.8 N`

## Ejecución individual de tests

```bash
cd /workspaces/uhtc-nanodb-/build
./tests/picogk_api_test
./tests/voxel_basic_test
./tests/uhtc_voxel_analysis_test
./tests/voxel_cuda_test
./tests/voxel_sdf_cuda_test
```

## Limpieza

```bash
rm -rf /workspaces/uhtc-nanodb-/build
```
