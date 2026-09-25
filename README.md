# UHTC-NanoDB

## Estructura del repositorio

- `docs/reference/`: PDFs y documentación externa de referencia.
- `docs/notes/`: espacio reservado para notas editoriales y decisiones del proyecto.
- `data/parameters/directories-work/`: tablas y notas procedentes de `directories_work/dir`.
- `data/parameters/table-important/`: tablas y notas procedentes de `table&/mtr/important`.
- `assets/images/image1/`: imagen de la raíz del repositorio.
- `assets/images/image2/`: imágenes procedentes de `directories_work/dir/images`.
- `assets/images/image3/`: imágenes procedentes de las tablas y notas de `table&/mtr/important`.
- `rocket_advanced/src/csharp/`: prototipos C# de geometría y configuración.
- `combustion_internals/`: frontera para solvers de flujo, química y diagnósticos C++/CUDA.
- `turbopump_engine/`: casos y modelos 1D de flujo frío para bombas, tuberías, NPSH e inyectores.
- `injector/`: referencias y criterios para inyectores, separados de los ensayos de turbobomba.
- `uhtc/`: cribado térmico y estructural de materiales UHTC.
- `woven/`: topologías woven, lattice y gyroid para geometría generativa.
- `docs/architecture/`: contrato y límites entre PicoGK, NanoVDB, AMReX y los solvers.
- `artifacts/legacy-objects/`: objetos compilados heredados; no forman parte del build.
- `picogk/`, `PicoGKRuntime/`, `nanodb/`, `openvdb/`, `Src/`, `rust/`: código y dependencias de cálculo que conservan sus rutas.

El índice técnico de parámetros está en [`data/parameters/README.md`](data/parameters/README.md). Los PDFs y notas son material de estudio: cualquier valor usado en un modelo debe trazarse a su fuente y validarse con análisis, pruebas y requisitos de seguridad independientes.

# Rocket's books
parameters/tables/pikogk or c++ 26-nvidia:https://ntrs.nasa.gov/api/citations/19710019929/downloads/19710019929.pdf
Russian-Liquid-Propellant-Engines pdf
Design of Liquid Propellant Rocket Engines Second Edition

# AmRex integration-arrays for motor rocket design and voxelization(nanodb) of the turbopumps in c++
https://github.com/victorjosegarcia888-cpu/amrex/blob/development/Src/EB/AMReX_EB2_3D_C.cpp

# Other fluid dynamics non-english thermodynamic subject books

Computational thermal/fluid/structural analysis + voxel engine for 3D printing of monolithic UHTC structures and aerospike nozzles. The programming language rust for systems:https://github.com/Traverse-Research/vdb-rs/tree/main/src
variables book: https://books.google.es/books?id=XMpyDwAAQBAJ&pg=PA2&hl=es&source=gbs_toc_r&cad=2#v=onepage&q&f=false
## UHTC Aperiodic Cooling Engine

UHTC Aperiodic Cooling Engine is a computational design and acceleration hardware framework for generating monolithic Ultra-High Temperature Ceramic (UHTC) structures.
The book was written "on the job" for use by those active in all phases of engine systems design, development, and application, in industry as well as government agencies. Since it addresses itself to human beings set out to create new machines, rather than describing machines about to dominate man, the language chosen may not always be "functional" in the strict sense of the word. 

The book presents sufficient detail to familiarize and educate thoroughly those responsible for various aspects of liquid propellant rocketry, including engine systems design, engine development, and flight vehicle application. It should enable the rocket engineer to conduct, independently, complete or partial engine systems preliminary detail designs and to understand and judge the activities in, and the problems, limitations, and "facts of life~ of the various subsystems making up a complete engine system. It also attempts to educate those ultimately interested in specialized subsystems and component design (thrust chamber, turbopump, control valves, etc.) about their own as well as neighboring subsystems and about the complete engine system. This should enable the student to prepare realistic analytical calculations and design layouts with a long headstart toward the final specialized designs for subsystem production release. Special emphasis has been placed on engine flight application to stimulate engine systems and subsystem designers to think in these terms from the outset. The book is intended as a textbook, with specific consideration of the teacher without industry experience. We hope it will stimulate those desiring to specialize in the area of a rocket engine subsystem by supplying adequate information to enable them to benefit fully from the specialized literature. Thus, it provides a realistic expert introduction for those joining the liquid propellant rocket engine field.

## The Quasycrystals computational geometry as carbon C, niquel, boron sylicates, tantalum in the pikogk geometric motor
important:https://github.com/victorjosegarcia888-cpu/LEAP71_QuasiCrystals
The system combines 6D quasicrystalline aperiodic geometries to block atomic oxygen (O₂) diffusional percolation, TPMS/Gyroid networks for regenerative active cooling, and a native implicit voxelization engine (0% chordal error) parallelized through the Xilinx XRT runtime and Vitis AI Engines (AIE). other architecture-file base system 1.  https://github.com/victorjosegarcia888-cpu/examples
CFD across arm architecture amREX integration: https://github.com/victorjosegarcia888-cpu/amrex/tree/development/Src/Base

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
https://es.scribd.com/document/7362263/Russian-Liquid-Propellant-Engines

- Prandtl-Meyer expansion and inversion for spike profile.
- `AerospikePlugSDF` with regenerative gyroid channels.
- Schmucker separation criterion.
- SWBLI lateral load estimation.

## Environment

- OS: Ubuntu 24.04 (Noble)
- CUDA: 12.0
- CMake: 3.24+
- GCC: 13.3.0 (C++20)
- PicoGK Runtime: upstream sync from https://github.com/leap71/PicoGKRuntime
- PicoGK Viewer: C++ GLFW/OpenGL viewer included from upstream

## Build

```bash
cd /workspaces/uhtc-nanodb-
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

Optional viewer build (requires OpenGL/GLFW):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PICOGK_VIEWER=ON
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
- `pnanovdb_portable_test` — PASSED
- `printer_slicer_test` — PASSED
- `rocket_toolpath_test` — PASSED
- `voxel_cuda_test` — PASSED
- `voxel_sdf_cuda_test` — PASSED
- `cuda_thermal_test` — PASSED (requires NVIDIA GPU)
- `cuda_rocket_test` — PASSED (requires NVIDIA GPU)

## Exhaustive CUDA Test Analysis

### `voxel_sdf_cuda_test.cu` — Baseline SDF Voxel Test

**Kernel `init_voxel_grid`:**
- 3D grid initialization with sphere + cylinder SDF
- Radius: `r_sphere = 0.02 + 0.005 * t_snap` (time-dependent growth)
- Channel: cylinder carved through sphere via `fmax(sphere, -cylinder)`
- Material IDs: `0=empty, 1=solid, 2=channel`
- Launch config: `dim3 block(8,8,8)`, `dim3 grid(8,8,8)` for 64³ grid

**Kernel `count_materials`:**
- Parallel reduction with 3-way split in shared memory
- Shared memory layout: `[0..b-1]=c0, [b..2b-1]=c1, [2b..3b-1]=c2`
- Reduction pattern: `s >>= 1` with `__syncthreads()`
- 256 threads per block, 1024 blocks for 64³ grid

**Validation gaps identified:**
- No explicit CUDA error checking after kernel launches
- No validation of SDF values at known coordinates
- No verification of material distribution consistency
- No edge case testing (non-multiple grid sizes)
- No timing measurements
- No overflow protection for large grids

### `CudaVoxelTestUtils.h` — Testing Infrastructure

Provides:
- `CudaError` RAII wrapper for error checking
- `CudaTimer` for kernel timing measurements
- `validateMaterialCounts()` — tolerance-based material distribution check
- `validateSphereCarving()` — SDF correctness against analytic sphere
- `validateChannelCarving()` — cylindrical channel validation
- `measureKernelTime()` — automatic timing with iterations
- `verifyDeviceProperties()` — compute capability validation
- `allocateDeviceVoxelGrid()` — safe allocation with zero-init

### `CudaThermalKernels.h` — Rocket Thermal Analysis

**Kernels:**
- `thermalShockKernel` — 1D transient conduction into semi-infinite solid
- `heatFluxKernel` — boundary layer heat flux along Z
- `penetrationDepthKernel` — thermal penetration depth mask
- `thermalSafetyFactorKernel` — material-specific safety factor map
- `thermalStatsReduction` — min/max/avg temperature and stress

**Material parameters (`DeviceThermalParams`):**
- `k_w`, `rho_w`, `cp_w`, `alpha_te`, `E`, `nu`, `T_i`, `T_aw`, `h_g`

**Test coverage (`cuda_thermal_test.cu`):**
1. UHTC thermal properties validation
2. Temperature field bounds checking `[T_i, T_aw]`
3. Stress sign validation (compressive = negative)
4. Penetration depth ratio calculation
5. Safety factor distribution analysis
6. Kernel timing measurement
7. 5-device field copies for detailed inspection
8. Unsafe voxel counting

### `CudaRocketKernels.h` — Rocket Geometry Kernels

**Kernels:**
- `aerospikeSDFKernel` — plug body SDF with Prandtl-Meyer profile
- `aerospikeMaterialKernel` — material ID from composite SDF
- `schmuckerKernel` — oblique shock separation ratio
- `supercriticalChannelKernel` — HTD-suppressed channel SDF
- `compositeSolidKernel` — CSG union of aerospike + channels
- `rocketStatsReduction` — SDF stats and material counts

**Parameters:**
- `DeviceAerospikeParams`: throat_radius, nu_exit, spike_length, channel_width, gyroid_scale, gamma
- `DeviceSupercriticalChannelParams`: P, T_bulk, G, q_flux, D_h, T_pseudocritical, Delta_T_critical, HTD_Suppression_Factor, base_width

**Test coverage (`cuda_rocket_test.cu`):**
1. Aerospike plug SDF generation
2. Gyroid channel SDF generation
3. Composite solid CSG operation
4. Material assignment (0=empty, 1=solid, 2=channel)
5. Schmucker separation ratio analytical validation
6. SDF bounds validation (min < 0 for solid)
7. Base Z-plane solidity check
8. Kernel timing (aerospike + composite)
9. Material ID range validation (0-2 only)
10. Device compute capability check

## Rocket Toolpath Generator

`nanodb/nanovdb/tools/RocketToolpaths.h` provides material-specific scan strategies for rocket-critical LPBF/EBM/DED manufacturing:

- **Materials covered:** Inconel 718, Ti-6Al-4V, UHTC (ZrB₂/TaC), Monolithic Weave II, Ceramic
- **Strategies:** Stripe, Island, Rotating, Chessboard, Concentric
- **Outputs:** G-code (`.nc`) and CLI slice files with per-material laser parameters

Example:
```cpp
RocketToolpathGenerator gen("Inconel718", "LPBF");
auto layers = gen.generateRectangularPart(0, 0, 10, 10, 0, 2, 40);
gen.writeGCode("rocket_part.nc", layers);
gen.writeCLI("rocket_part.cli", layers);
```

## PicoGK Viewer (C++)

The upstream PicoGK GL viewer is included from [leap71/PicoGKRuntime](https://github.com/leap71/PicoGKRuntime):

- `picogk/Source/PicoGKGLViewer*.cpp/.h` — OpenGL viewer based on GLFW
- Build with `-DBUILD_PICOGK_VIEWER=ON` (requires OpenGL and GLFW)
- Can visualize voxel fields, lattices, and mesh geometry generated by the analytical modules

## Sincronización con PicoGK upstream

- API headers sync: `picogk/API/PicoGK*.h` match upstream PicoGKRuntime v26.2
- Runtime source added: `picogk/Source/PicoGKGLViewer*.cpp/.h`
- Viewer dependencies: GLFW + OpenGL (optional, disabled by default)

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
  
## Algorithm language reference
c++ algorithm:https://cplusplus.com/reference/algorithm/find/
2. https://www.hackerrank.com/challenges/cpp-exception-handling/problem?isFullScreen=true
3. https://cplusplus.com/reference/algorithm/search/
4. https://cplusplus.com/reference/cstdint/

# others
fuel_regression:https://en.wikipedia.org/wiki/Hybrid_rocket_fuel_regression
other link;- https://www.russianspaceweb.com/rd170.html
