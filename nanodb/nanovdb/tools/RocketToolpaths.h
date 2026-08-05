#ifndef NANOVDB_TOOLS_ROCKET_TOOLPATHS_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_ROCKET_TOOLPATHS_H_HAS_BEEN_INCLUDED

#include <nanovdb/NanoVDB.h>
#include <picogk/API/PicoGKApiTypes.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace nanovdb {
namespace tools {

/// @brief Toolpath strategies for LPBF/EBM of rocket-critical materials
enum class ToolpathStrategy {
    Stripe,           // Standard stripe scan
    Island,           // Island scan for reduced heat accumulation
    Rotating,         // Rotating scan strategy
    Chessboard,       // Chessboard for complex geometries
    Concentric        // Concentric for cylindrical parts
};

/// @brief Laser scan parameters
struct LaserScanParams {
    double laser_power_W;
    double scan_speed_mm_s;
    double hatch_spacing_mm;
    double layer_thickness_um;
    double spot_size_um;
    double overlap_percent;
    ToolpathStrategy strategy;
    bool use_adaptive_hatch;
    double min_hatch_spacing_um;
    double max_hatch_spacing_um;
};

/// @brief Material-specific laser parameters for rocket-critical materials
inline LaserScanParams getMaterialLaserParams(const std::string& material_name,
                                               const std::string& printer_family) {
    LaserScanParams params = {200.0, 1000.0, 0.1, 50.0, 80.0, 25.0, ToolpathStrategy::Stripe, false, 50.0, 200.0};

    if (material_name == "Inconel718" && printer_family == "LPBF") {
        params.laser_power_W = 350.0;
        params.scan_speed_mm_s = 1200.0;
        params.hatch_spacing_mm = 0.12;
        params.layer_thickness_um = 40.0;
        params.spot_size_um = 80.0;
        params.overlap_percent = 25.0;
        params.strategy = ToolpathStrategy::Rotating;
    } else if (material_name == "Inconel718" && printer_family == "DED") {
        params.laser_power_W = 1800.0;
        params.scan_speed_mm_s = 800.0;
        params.hatch_spacing_mm = 0.3;
        params.layer_thickness_um = 100.0;
        params.spot_size_um = 200.0;
        params.overlap_percent = 20.0;
        params.strategy = ToolpathStrategy::Stripe;
    } else if (material_name == "Ti64" && printer_family == "LPBF") {
        params.laser_power_W = 280.0;
        params.scan_speed_mm_s = 1300.0;
        params.hatch_spacing_mm = 0.12;
        params.layer_thickness_um = 30.0;
        params.spot_size_um = 70.0;
        params.overlap_percent = 20.0;
        params.strategy = ToolpathStrategy::Island;
    } else if (material_name == "Ti64" && printer_family == "EBM") {
        params.laser_power_W = 1500.0;  // EBM beam power
        params.scan_speed_mm_s = 7000.0;
        params.hatch_spacing_mm = 0.15;
        params.layer_thickness_um = 50.0;
        params.spot_size_um = 120.0;
        params.overlap_percent = 15.0;
        params.strategy = ToolpathStrategy::Stripe;
    } else if (material_name == "UHTC") {
        params.laser_power_W = 400.0;
        params.scan_speed_mm_s = 600.0;
        params.hatch_spacing_mm = 0.08;
        params.layer_thickness_um = 25.0;
        params.spot_size_um = 60.0;
        params.overlap_percent = 30.0;
        params.strategy = ToolpathStrategy::Chessboard;
        params.use_adaptive_hatch = true;
    } else if (material_name == "Monolithic_Weave_II") {
        params.laser_power_W = 380.0;
        params.scan_speed_mm_s = 700.0;
        params.hatch_spacing_mm = 0.08;
        params.layer_thickness_um = 30.0;
        params.spot_size_um = 70.0;
        params.overlap_percent = 28.0;
        params.strategy = ToolpathStrategy::Chessboard;
        params.use_adaptive_hatch = true;
    } else if (material_name == "Ceramic_Borosilicate") {
        params.laser_power_W = 120.0;
        params.scan_speed_mm_s = 1500.0;
        params.hatch_spacing_mm = 0.05;
        params.layer_thickness_um = 20.0;
        params.spot_size_um = 50.0;
        params.overlap_percent = 35.0;
        params.strategy = ToolpathStrategy::Concentric;
    }

    return params;
}

/// @brief A single scan vector in the toolpath
struct ScanVector {
    PKVector3 start;
    PKVector3 end;
    double speed_mm_s;
    double laser_power_W;
    int layer_index;
};

/// @brief A complete layer toolpath
struct LayerToolpath {
    int layer_index;
    double z_height_mm;
    std::vector<ScanVector> scan_vectors;
    double bounding_box_min_x, bounding_box_min_y;
    double bounding_box_max_x, bounding_box_max_y;
};

/// @brief Generates stripe-based toolpaths for a rectangular region
inline std::vector<ScanVector> generateStripeToolpath(double x_min, double y_min,
                                                       double x_max, double y_max,
                                                       double z,
                                                       const LaserScanParams& params,
                                                       int layer_index) {
    std::vector<ScanVector> vectors;
    double hatch = params.hatch_spacing_mm;
    int num_stripes = static_cast<int>(std::ceil((y_max - y_min) / hatch));

    bool direction = true;
    for (int i = 0; i < num_stripes; ++i) {
        double y = y_min + i * hatch;
        if (y > y_max) y = y_max;

        ScanVector sv;
        sv.layer_index = layer_index;
        sv.speed_mm_s = params.scan_speed_mm_s;
        sv.laser_power_W = params.laser_power_W;

        if (direction) {
            sv.start = {static_cast<float>(x_min), static_cast<float>(y), static_cast<float>(z)};
            sv.end = {static_cast<float>(x_max), static_cast<float>(y), static_cast<float>(z)};
        } else {
            sv.start = {static_cast<float>(x_max), static_cast<float>(y), static_cast<float>(z)};
            sv.end = {static_cast<float>(x_min), static_cast<float>(y), static_cast<float>(z)};
        }

        vectors.push_back(sv);
        direction = !direction;
    }

    return vectors;
}

/// @brief Generates island-based toolpaths
inline std::vector<ScanVector> generateIslandToolpath(double x_min, double y_min,
                                                       double x_max, double y_max,
                                                       double z,
                                                       const LaserScanParams& params,
                                                       int layer_index) {
    std::vector<ScanVector> vectors;
    double island_size = 5.0; // mm
    double hatch = params.hatch_spacing_mm;

    for (double island_y = y_min; island_y < y_max; island_y += island_size) {
        bool direction = true;
        for (double y = island_y; y < std::min(island_y + island_size, y_max); y += hatch) {
            ScanVector sv;
            sv.layer_index = layer_index;
            sv.speed_mm_s = params.scan_speed_mm_s;
            sv.laser_power_W = params.laser_power_W;

            if (direction) {
                sv.start = {static_cast<float>(x_min), static_cast<float>(y), static_cast<float>(z)};
                sv.end = {static_cast<float>(x_max), static_cast<float>(y), static_cast<float>(z)};
            } else {
                sv.start = {static_cast<float>(x_max), static_cast<float>(y), static_cast<float>(z)};
                sv.end = {static_cast<float>(x_min), static_cast<float>(y), static_cast<float>(z)};
            }

            vectors.push_back(sv);
            direction = !direction;
        }
    }

    return vectors;
}

/// @brief Generates chessboard-based toolpaths for complex geometries
inline std::vector<ScanVector> generateChessboardToolpath(double x_min, double y_min,
                                                           double x_max, double y_max,
                                                           double z,
                                                           const LaserScanParams& params,
                                                           int layer_index) {
    std::vector<ScanVector> vectors;
    double cell_size = 2.0; // mm
    double hatch = params.hatch_spacing_mm;

    for (double cx = x_min; cx < x_max; cx += cell_size) {
        for (double cy = y_min; cy < y_max; cy += cell_size) {
            bool parity = ((static_cast<int>(cx / cell_size) + static_cast<int>(cy / cell_size)) % 2 == 0);
            if (!parity) continue;

            bool direction = true;
            for (double y = cy; y < std::min(cy + cell_size, y_max); y += hatch) {
                ScanVector sv;
                sv.layer_index = layer_index;
                sv.speed_mm_s = params.scan_speed_mm_s;
                sv.laser_power_W = params.laser_power_W;

                if (direction) {
                    sv.start = {static_cast<float>(cx), static_cast<float>(y), static_cast<float>(z)};
                    sv.end = {static_cast<float>(std::min(cx + cell_size, x_max)), static_cast<float>(y), static_cast<float>(z)};
                } else {
                    sv.start = {static_cast<float>(std::min(cx + cell_size, x_max)), static_cast<float>(y), static_cast<float>(z)};
                    sv.end = {static_cast<float>(cx), static_cast<float>(y), static_cast<float>(z)};
                }

                vectors.push_back(sv);
                direction = !direction;
            }
        }
    }

    return vectors;
}

/// @brief Generates concentric toolpaths for cylindrical parts
inline std::vector<ScanVector> generateConcentricToolpath(double cx, double cy,
                                                           double inner_r, double outer_r,
                                                           double z,
                                                           const LaserScanParams& params,
                                                           int layer_index) {
    std::vector<ScanVector> vectors;
    double hatch = params.hatch_spacing_mm;
    int num_rings = static_cast<int>(std::ceil((outer_r - inner_r) / hatch));

    for (int i = 0; i < num_rings; ++i) {
        double r = inner_r + i * hatch;
        if (r > outer_r) r = outer_r;

        int num_segments = static_cast<int>(std::ceil(2.0 * M_PI * r / hatch));
        for (int s = 0; s < num_segments; ++s) {
            double a0 = 2.0 * M_PI * s / num_segments;
            double a1 = 2.0 * M_PI * (s + 1) / num_segments;

            ScanVector sv;
            sv.layer_index = layer_index;
            sv.speed_mm_s = params.scan_speed_mm_s;
            sv.laser_power_W = params.laser_power_W;
            sv.start = {static_cast<float>(cx + r * std::cos(a0)), static_cast<float>(cy + r * std::sin(a0)), static_cast<float>(z)};
            sv.end = {static_cast<float>(cx + r * std::cos(a1)), static_cast<float>(cy + r * std::sin(a1)), static_cast<float>(z)};
            vectors.push_back(sv);
        }
    }

    return vectors;
}

/// @brief Generates complete toolpaths for a part bounding box
class RocketToolpathGenerator {
public:
    RocketToolpathGenerator(const std::string& material_name, const std::string& printer_family)
        : mLaserParams(getMaterialLaserParams(material_name, printer_family))
    {}

    const LaserScanParams& laserParams() const { return mLaserParams; }

    /// @brief Generate toolpaths for a rectangular part region
    std::vector<LayerToolpath> generateRectangularPart(double x_min, double y_min,
                                                        double x_max, double y_max,
                                                        double z_min, double z_max,
                                                        double layer_thickness_um) {
        std::vector<LayerToolpath> layers;
        double z_step = layer_thickness_um / 1000.0;
        int num_layers = static_cast<int>(std::ceil((z_max - z_min) / z_step));

        for (int i = 0; i < num_layers; ++i) {
            double z = z_min + i * z_step;
            LayerToolpath layer;
            layer.layer_index = i;
            layer.z_height_mm = z;
            layer.bounding_box_min_x = x_min;
            layer.bounding_box_min_y = y_min;
            layer.bounding_box_max_x = x_max;
            layer.bounding_box_max_y = y_max;

            switch (mLaserParams.strategy) {
                case ToolpathStrategy::Island:
                    layer.scan_vectors = generateIslandToolpath(x_min, y_min, x_max, y_max, z, mLaserParams, i);
                    break;
                case ToolpathStrategy::Chessboard:
                    layer.scan_vectors = generateChessboardToolpath(x_min, y_min, x_max, y_max, z, mLaserParams, i);
                    break;
                case ToolpathStrategy::Concentric:
                    layer.scan_vectors = generateConcentricToolpath((x_min + x_max) * 0.5, (y_min + y_max) * 0.5,
                                                                    0.0, (x_max - x_min) * 0.5, z, mLaserParams, i);
                    break;
                case ToolpathStrategy::Rotating:
                case ToolpathStrategy::Stripe:
                default:
                    layer.scan_vectors = generateStripeToolpath(x_min, y_min, x_max, y_max, z, mLaserParams, i);
                    break;
            }

            layers.push_back(layer);
        }

        return layers;
    }

    /// @brief Write toolpaths as simple G-code
    bool writeGCode(const std::string& filepath,
                    const std::vector<LayerToolpath>& layers) const {
        std::ofstream out(filepath);
        if (!out.is_open()) return false;

        out << "; Generated by NanoDB RocketToolpathGenerator\n";
        out << "; Material parameters:\n";
        out << ";   Laser power: " << mLaserParams.laser_power_W << " W\n";
        out << ";   Scan speed: " << mLaserParams.scan_speed_mm_s << " mm/s\n";
        out << ";   Hatch spacing: " << mLaserParams.hatch_spacing_mm << " mm\n";
        out << ";   Layer thickness: " << mLaserParams.layer_thickness_um << " um\n";
        out << "G28 ; Home\n";
        out << "G90 ; Absolute positioning\n";
        out << "M3 S0 ; Laser off\n";

        for (const auto& layer : layers) {
            out << "\n; Layer " << layer.layer_index << " Z=" << layer.z_height_mm << " mm\n";
            out << "G0 Z" << layer.z_height_mm << " F3000\n";

            for (const auto& sv : layer.scan_vectors) {
                out << "G0 X" << sv.start.X << " Y" << sv.start.Y << " F" << sv.speed_mm_s << "\n";
                out << "M3 S" << static_cast<int>(sv.laser_power_W * 10) << " ; Laser on\n";
                out << "G1 X" << sv.end.X << " Y" << sv.end.Y << " F" << sv.speed_mm_s << "\n";
                out << "M3 S0 ; Laser off\n";
            }
        }

        out << "M30 ; End of program\n";
        out.close();
        return true;
    }

    /// @brief Write toolpaths as CLI slices
    bool writeCLI(const std::string& filepath,
                  const std::vector<LayerToolpath>& layers) const {
        std::ofstream out(filepath);
        if (!out.is_open()) return false;

        out << "$$HEADERSTART\n";
        out << "$$VERSION " << 1.0 << "\n";
        out << "$$UNITS mm\n";
        out << "$$LAYERTHICK " << (mLaserParams.layer_thickness_um / 1000.0) << "\n";
        out << "$$HEADEREND\n";

        for (const auto& layer : layers) {
            out << "\n$$LAYER " << layer.layer_index << "\n";
            out << "$$CONTOUR\n";
            for (const auto& sv : layer.scan_vectors) {
                out << "$$LINE " << sv.start.X << " " << sv.start.Y << " " << sv.end.X << " " << sv.end.Y << "\n";
            }
            out << "$$ENDCONTOUR\n";
        }

        out.close();
        return true;
    }

    /// @brief Print toolpath summary
    void printSummary() const {
        printf("Rocket Toolpath Generator\n");
        printf("Laser power: %.1f W\n", mLaserParams.laser_power_W);
        printf("Scan speed: %.1f mm/s\n", mLaserParams.scan_speed_mm_s);
        printf("Hatch spacing: %.3f mm\n", mLaserParams.hatch_spacing_mm);
        printf("Layer thickness: %.1f um\n", mLaserParams.layer_thickness_um);
        printf("Strategy: %d\n", static_cast<int>(mLaserParams.strategy));
    }

private:
    LaserScanParams mLaserParams;
};

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_ROCKET_TOOLPATHS_H_HAS_BEEN_INCLUDED
