# UHTC-NanoDB

Motor de análisis térmico/fluidos/estructural + voxeles para impresión 3D de UHTC monolíticos y toberas aerospike.

## Entorno

- OS: Ubuntu 24.04 (Noble)
- CUDA: 12.0 (`nvcc` presente; no hay GPU física en codespace)
- CMake: 3.28.3
- GCC: 13.3.0 (C++20)

## Módulos analíticos

### 1. Choque térmico transitorio UHTC
`nanodb/nanovdb/tools/TransientThermalShock.h`
- Conducción 1D transitoria en sólido semi-infinito.
- Penetración térmica `δ_th(t) ≈ 2√(α·t)`.
- Temperatura superficial analítica `T_gas(t)`.
- Tensión de compresión transitoria.
- Clase `TransientThermalShockVoxels<BuildT>` para generar grid base.

### 2. Canales micro-mapeados supercríticos
`nanodb/nanovdb/tools/SupercriticalChannel.h`
- Estado de refrigerante LH2/LCH4 supercrítico.
- Correlación de Nusselt modificada de Jackson.
- Detección de riesgo HTD.
- `MicroMappedChannelSDF` para modulación implícita de ancho de canal.
- Integración CSG con `compositeSolidWithChannels`.

### 3. Tobera aerospike + SWBLI
`nanodb/nanovdb/tools/AerospikeNozzle.h`
- Función de Prandtl-Meyer e inversión para perfil de espiga.
- `AerospikePlugSDF`: cuerpo de espiga + canales gyroid.
- Criterio de Schmucker para separación de choque.
- Estimación de carga lateral SWBLI.

## Dependencias

```bash
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libboost-all-dev libtbb-dev \
  libopenexr-dev libilmbase-dev \
  libpng-dev libjpeg-dev \
  libx11-dev libxi-dev libgl-dev libglew-dev libglfw3-dev \
  libcub-dev nvidia-cuda-toolkit
```

## Compilación

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## Tests

```bash
cd build
ctest --output-on-failure
```

Tests incluidos:
- `picogk_api_test` — tipos PicoGK
- `voxel_basic_test` — headers NanoVDB/nanodb/CUDA
- `uhtc_voxel_analysis_test` — térmico, fluido supercrítico, aerospike
- `voxel_cuda_test` — compilación CUDA + runtime sin GPU

## Notas

- Los módulos analíticos actuales implementan formulación y validaciones unitarias.
- La serialización completa a NanoVDB se integra vía `nanovdb::tools::CreateNanoGrid` cuando se enlace OpenVDB/NanoVDB compilado.
- Para ejecución CUDA real, usar máquina con GPU NVIDIA + drivers.
