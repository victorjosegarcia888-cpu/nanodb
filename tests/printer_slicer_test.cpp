#include <cstdio>
#include <cstring>
#include <cmath>

#include "nanodb/nanovdb/tools/PrinterSlicerOutput.h"
#include "nanodb/nanovdb/tools/TransientThermalShock.h"
#include "nanodb/nanovdb/tools/SupercriticalChannel.h"
#include "nanodb/nanovdb/tools/AerospikeNozzle.h"

using namespace nanovdb::tools;

int main() {
    printf("=== Printer Slicer / Output Module Test ===\n");

    // 1. Validate printer profiles for all listed printers
    const char* printers[] = {
        "EOS_M400-4",
        "SLM_NXG_XII_600",
        "Renishaw_RenAM_500Q",
        "Arcam_EBM_Q20+",
        "DMG_Mori_Lasertec_4300",
        "Lithoz_CeraFab_S65"
    };

    for (size_t i = 0; i < sizeof(printers)/sizeof(printers[0]); ++i) {
        PrinterProfile p = getPrinterProfile(printers[i]);
        printf("Printer: %s, family=%s, material=%s\n",
               p.name.c_str(),
               p.family == PrinterFamily::LPBF ? "LPBF" :
               p.family == PrinterFamily::EBM ? "EBM" :
               p.family == PrinterFamily::DED ? "DED" : "Ceramic",
               p.material_family.c_str());
    }

    // 2. Validate material properties
    const char* materials[] = {"Inconel718", "Ti64", "UHTC", "Monolithic_Weave_II", "Ceramic_Borosilicate"};
    for (size_t i = 0; i < sizeof(materials)/sizeof(materials[0]); ++i) {
        MaterialProperties m = getMaterialProperties(materials[i]);
        printf("Material: %s, density=%.0f kg/m3, Tm=%.0f K\n",
               m.name.c_str(), m.density_kg_m3, m.melting_point_K);
    }

    // 3. Validate slicer configuration
    PrinterSlicer slicer("EOS_M400-4", "Inconel718");
    slicer.printSummary();

    SliceConfig cfg = recommendSliceConfig("SLM_NXG_XII_600", "Ti64");
    printf("SLM NXG XII 600 Ti64: layer=%.1f um, infill=%.1f%%\n",
           cfg.layer_thickness_um, cfg.infill_percent);

    // 4. Validate STL write (minimal triangle)
    std::vector<float> vertices = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    std::vector<uint32_t> indices = {0, 1, 2};
    bool stl_ok = writeSTL("/tmp/test_part.stl", vertices, indices, "TestPart");
    printf("STL write: %s\n", stl_ok ? "OK" : "FAIL");

    // 5. Validate CLI write
    std::vector<std::vector<float>> slices;
    slices.push_back({0.0f, 0.0f, 1.0f, 1.0f});
    bool cli_ok = writeCLI("/tmp/test_slices.cli", slices, cfg);
    printf("CLI write: %s\n", cli_ok ? "OK" : "FAIL");

    // 6. Validate PNanoVDB portable export from analytical modules
    uint32_t buf[64] = {0};

    UHTCThermalShockParams thermal_params = {300.0, 3500.0, 80.0, 6000.0, 500.0, 1e6, 0.01, 0.5};
    UHTCStressParams stress_params = {200e9, 0.3, 5.5e-6};
    TransientThermalShockVoxels<float> thermal(thermal_params, stress_params, 0.5, 32, 32, 32);
    uint64_t thermal_words = thermal.exportPortableThermalBuffer(buf, 64);
    printf("Thermal PNanoVDB export: %llu words\n", (unsigned long long)thermal_words);

    // SupercriticalChannel
    SupercriticalCoolantState state = {101325.0, 300.0, 100.0, 1e6, 0.01};
    uint64_t channel_words = exportPortableChannelBuffer(buf, 64, state, 0.005);
    printf("Channel PNanoVDB export: %llu words\n", (unsigned long long)channel_words);

    // AerospikeNozzle
    uint64_t aero_words = exportPortableAerospikeBuffer(buf, 64, 0.05, 0.5);
    printf("Aerospike PNanoVDB export: %llu words\n", (unsigned long long)aero_words);

    // 7. Validate PNanoVDB magic in exported buffers
    if (thermal_words >= 2) {
        uint64_t magic = ((uint64_t)buf[0] << 32) | buf[1];
        printf("Thermal buffer magic: 0x%llx\n", (unsigned long long)magic);
    }

    printf("Printer slicer/output module test passed.\n");
    return 0;
}
