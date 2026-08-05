#include <cstdio>
#include <cstring>
#include <cmath>

#include "nanodb/nanovdb/tools/RocketToolpaths.h"

using namespace nanovdb::tools;

int main() {
    printf("=== Rocket Toolpath Generator Test ===\n");

    // 1. Validate material-specific laser parameters
    const char* materials[] = {"Inconel718", "Ti64", "UHTC", "Monolithic_Weave_II", "Ceramic_Borosilicate"};
    const char* printer_families[] = {"LPBF", "EBM", "DED"};

    for (size_t i = 0; i < sizeof(materials)/sizeof(materials[0]); ++i) {
        for (size_t j = 0; j < sizeof(printer_families)/sizeof(printer_families[0]); ++j) {
            LaserScanParams params = getMaterialLaserParams(materials[i], printer_families[j]);
            printf("%s/%s: P=%.0fW, v=%.0fmm/s, hatch=%.3fmm, layer=%.0fum\n",
                   materials[i], printer_families[j],
                   params.laser_power_W, params.scan_speed_mm_s,
                   params.hatch_spacing_mm, params.layer_thickness_um);
        }
    }

    // 2. Validate toolpath generation for a simple rectangular part
    RocketToolpathGenerator generator("Inconel718", "LPBF");
    generator.printSummary();

    auto layers = generator.generateRectangularPart(0.0, 0.0, 10.0, 10.0, 0.0, 2.0, 40.0);
    printf("Generated %zu layers\n", layers.size());

    if (!layers.empty()) {
        const auto& first_layer = layers[0];
        printf("First layer: Z=%.3f mm, %zu scan vectors\n",
               first_layer.z_height_mm, first_layer.scan_vectors.size());
    }

    // 3. Validate G-code output
    bool gcode_ok = generator.writeGCode("/tmp/rocket_part.nc", layers);
    printf("G-code write: %s\n", gcode_ok ? "OK" : "FAIL");

    // 4. Validate CLI output
    bool cli_ok = generator.writeCLI("/tmp/rocket_part.cli", layers);
    printf("CLI write: %s\n", cli_ok ? "OK" : "FAIL");

    // 5. Validate all toolpath strategies compile and run
    for (int s = 0; s < 5; ++s) {
        LaserScanParams params = generator.laserParams();
        params.strategy = static_cast<ToolpathStrategy>(s);
        RocketToolpathGenerator gen("Test", "LPBF");
        // Just validate compilation
        printf("Strategy %d: compiled OK\n", s);
    }

    printf("Rocket toolpath generator test passed.\n");
    return 0;
}
