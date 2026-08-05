# UHTC-NanoDB

Computational thermal/fluid/structural analysis + voxel engine for 3D printing of monolithic UHTC structures and aerospike nozzles.

## UHTC Aperiodic Cooling Engine

UHTC Aperiodic Cooling Engine is a computational design and acceleration hardware framework for generating monolithic Ultra-High Temperature Ceramic (UHTC) structures.

The system combines 6D quasicrystalline aperiodic geometries to block atomic oxygen (O₂) diffusional percolation, TPMS/Gyroid networks for regenerative active cooling, and a native implicit voxelization engine (0% chordal error) parallelized through the Xilinx XRT runtime and Vitis AI Engines (AIE).

## 3D Printing Engine 🖨️

The 3D printing module is the heart of the manufacturing system. It defines how parts are fabricated, generates voxels, slices, thermal fields, and produces the files that industrial printers actually use.

### Supported Printers

- **EOS M400-4** (LPBF)
- **SLM Solutions NXG XII 600** (LPBF)
- **Renishaw RenAM 500Q** (LPBF)
- **Arcam EBM Q20+** (EBM)
- **DMG Mori Lasertec 4300** (Hybrid DED)
- **Lithoz CeraFab S65** (Advanced Ceramics)

### Supported Materials

- **Inconel 718**
- **Ti-6Al-4V (Ti64)**
- **UHTC (ZrB₂/TaC)**
- **Monolithic weave II**
- **Ceramic (modified boron silicate)**

## Analytical Modules

### 1. UHTC Transient Thermal Shock
`nanodb/nanovdb/tools/TransientThermalShock.h`

- 1D transient conduction, semi-infinite solid.
- Thermal penetration depth `δ_th(t) ≈ 2√(α·t)`.
- Analytical surface temperature `T_gas(t)`.
- Transient compressive stress.
- `TransientThermalShockVoxels<BuildT>` class for base grid generation.

### 2. Supercritical Micro-Mapped Channels
`nanodb/nanovdb/tools/SupercriticalChannel.h`

- LH2/LCH4 supercritical coolant state.
- Modified Jackson Nusselt correlation.
- HTD risk detection.
- `MicroMappedChannelSDF` for implicit channel width modulation.
- CSG integration via `compositeSolidWithChannels`.

### 3. Aerospike Nozzle + SWBLI
`nanodb/nanovdb/tools/AerospikeNozzle.h`

- Prandtl-Meyer expansion and inversion for spike profile.
- `AerospikePlugSDF` with regenerative gyroid channels.
- Schmucker separation criterion.
- SWBLI lateral load estimation.

## Environment

- OS: Ubuntu 24.04 (Noble)
- CUDA: 12.0
- CMake: 3.28.3
- GCC: 13.3.0 (C++20)

## Build

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

Included tests:
- `picogk_api_test` — PASSED
- `voxel_basic_test` — PASSED
- `uhtc_voxel_analysis_test` — PASSED
- `voxel_cuda_test` — PASSED
- `voxel_sdf_cuda_test` — PASSED

## Quantitative Analysis

```bash
python3 /workspaces/uhtc-nanodb-/analysis/uhtc_analysis.py
```

Key results for UHTC (k=80 W/mK, ρ=6000 kg/m³, cp=500 J/kgK):
- `alpha ≈ 2.67e-5 m²/s`
- `delta_th(500 ms) ≈ 7.3 mm < t_w = 20 mm` → semi-infinite assumption valid
- `T_gas(500 ms) ≈ 2770 K`
- `sigma_th(500 ms) ≈ -6294 MPa` → yield reached at ~1 ms

## Real NVIDIA GPU Setup

### Automated Setup Script

```bash
sudo bash setup_cuda_env.sh
```

This installs: apt prerequisites, CUDA repository, toolkit, builds the project, runs tests, and executes the Python analysis. Log is written to `setup_cuda_env.log`.

### NVIDIA Machine Options

#### Local WSL2 (lightest option)

In PowerShell as Administrator:
```powershell
wsl --install -d Ubuntu-24.04
```

Then inside WSL2:
```bash
sudo bash setup_cuda_env.sh
```

Or manually:
```bash
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libboost-all-dev libtbb-dev \
  libopenexr-dev libilmbase-dev \
  libpng-dev libjpeg-dev \
  libx11-dev libxi-dev libgl-dev libglew-dev libglfw3-dev \
  libcub-dev nvidia-cuda-toolkit

cd /workspaces/uhtc-nanodb-
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
ctest --output-on-failure
```

#### Low-Cost Cloud GPU (RunPod / Vast.ai / Lambda Labs)

- Launch an instance with T4 or RTX 4090.
- Clone the repo and run the same `setup_cuda_env.sh`.
- No local installation required.

### One-Command Setup (WSL2/Cloud)

```bash
git clone <your-repo>
cd uhtc-nanodb-
sudo bash setup_cuda_env.sh
ctest --output-on-failure
```

## Project Status

- 5 tests compiling and passing in codespace.
- CUDA 12.0 toolkit installed; kernels compile successfully.
- Setup script ready for WSL2 or cloud NVIDIA GPU environments.
- PicoGK Runtime exposes a C API; call from C# via P/Invoke.
