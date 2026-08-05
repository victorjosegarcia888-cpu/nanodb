#ifndef NANOVDB_TOOLS_PRINTER_SLICER_OUTPUT_H_HAS_BEEN_INCLUDED
#define NANOVDB_TOOLS_PRINTER_SLICER_OUTPUT_H_HAS_BEEN_INCLUDED

#include <nanovdb/NanoVDB.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace nanovdb {
namespace tools {

/// @brief Supported printer families
enum class PrinterFamily {
    LPBF,    // Laser Powder Bed Fusion
    EBM,     // Electron Beam Melting
    DED,     // Directed Energy Deposition
    Ceramic  // Advanced ceramic binder jetting / lithography
};

/// @brief Per-printer capabilities and constraints
struct PrinterProfile {
    std::string name;
    PrinterFamily family;
    double min_layer_thickness_um;     // micrometers
    double max_layer_thickness_um;
    double min_feature_size_um;
    double max_build_volume_x_mm;
    double max_build_volume_y_mm;
    double max_build_volume_z_mm;
    bool supports_support_scaffold;
    bool requires_chamber_inert;
    std::string native_file_format;    // STL, CLI, SLI, etc.
    std::string material_family;
};

/// @brief Known printer profiles
inline PrinterProfile getPrinterProfile(const std::string& printer_name) {
    if (printer_name == "EOS_M400-4") {
        return {"EOS M400-4", PrinterFamily::LPBF, 30.0, 150.0, 80.0, 250.0, 250.0, 350.0, true, true, "STL", "Inconel/Ti64"};
    }
    if (printer_name == "SLM_NXG_XII_600") {
        return {"SLM Solutions NXG XII 600", PrinterFamily::LPBF, 30.0, 150.0, 60.0, 600.0, 600.0, 600.0, true, true, "STL", "Inconel/Ti64"};
    }
    if (printer_name == "Renishaw_RenAM_500Q") {
        return {"Renishaw RenAM 500Q", PrinterFamily::LPBF, 25.0, 100.0, 70.0, 125.0, 125.0, 500.0, true, true, "STL", "Inconel/Ti64"};
    }
    if (printer_name == "Arcam_EBM_Q20+") {
        return {"Arcam EBM Q20+", PrinterFamily::EBM, 50.0, 200.0, 100.0, 200.0, 200.0, 350.0, false, true, "STL", "Ti64"};
    }
    if (printer_name == "DMG_Mori_Lasertec_4300") {
        return {"DMG Mori Lasertec 4300", PrinterFamily::DED, 100.0, 500.0, 200.0, 500.0, 300.0, 1000.0, true, true, "STL", "Inconel/Ti64"};
    }
    if (printer_name == "Lithoz_CeraFab_S65") {
        return {"Lithoz CeraFab S65", PrinterFamily::Ceramic, 10.0, 50.0, 30.0, 100.0, 100.0, 150.0, true, false, "STL", "Ceramic"};
    }
    // Default fallback
    return {"Generic", PrinterFamily::LPBF, 20.0, 200.0, 50.0, 200.0, 200.0, 200.0, true, true, "STL", "Generic"};
}

/// @brief Material database for 3D printing
struct MaterialProperties {
    std::string name;
    double density_kg_m3;
    double thermal_conductivity_W_mK;
    double specific_heat_J_kgK;
    double thermal_expansion_1K;
    double youngs_modulus_Pa;
    double poisson_ratio;
    double melting_point_K;
    std::string printer_family;
};

inline MaterialProperties getMaterialProperties(const std::string& material_name) {
    if (material_name == "Inconel718") {
        return {"Inconel 718", 8190.0, 11.4, 435.0, 13.0e-6, 200e9, 0.29, 1608.0, "LPBF/DED"};
    }
    if (material_name == "Ti64") {
        return {"Ti-6Al-4V", 4420.0, 6.7, 526.0, 9.2e-6, 114e9, 0.31, 1933.0, "LPBF/EBM/DED"};
    }
    if (material_name == "UHTC") {
        return {"ZrB2/TaC", 6000.0, 80.0, 500.0, 5.5e-6, 380e9, 0.18, 3500.0, "LPBF/Ceramic"};
    }
    if (material_name == "Monolithic_Weave_II") {
        return {"Monolithic Weave II", 5800.0, 65.0, 480.0, 5.8e-6, 360e9, 0.19, 3400.0, "LPBF/Ceramic"};
    }
    if (material_name == "Ceramic_Borosilicate") {
        return {"Modified Borosilicate", 2200.0, 1.2, 830.0, 8.5e-6, 65e9, 0.20, 1500.0, "Ceramic"};
    }
    return {"Unknown", 8000.0, 10.0, 500.0, 12.0e-6, 180e9, 0.3, 1800.0, "Generic"};
}

/// @brief Slice configuration
struct SliceConfig {
    double layer_thickness_um;
    double infill_percent;
    double support_spacing_mm;
    int support_tower_diameter;
    bool generate_supports;
    bool optimize_support;
    double print_speed_mm_s;
    double laser_power_W;
    double scan_speed_mm_s;
    double hatch_spacing_mm;
};

/// @brief Compute recommended slice config for a given printer and material
inline SliceConfig recommendSliceConfig(const std::string& printer_name, const std::string& material_name) {
    PrinterProfile printer = getPrinterProfile(printer_name);
    MaterialProperties mat = getMaterialProperties(material_name);

    SliceConfig cfg;
    cfg.layer_thickness_um = printer.min_layer_thickness_um * 2.0;
    if (cfg.layer_thickness_um > printer.max_layer_thickness_um) {
        cfg.layer_thickness_um = printer.max_layer_thickness_um * 0.8;
    }
    cfg.infill_percent = 100.0;
    cfg.support_spacing_mm = 0.5;
    cfg.support_tower_diameter = 1;
    cfg.generate_supports = printer.supports_support_scaffold;
    cfg.optimize_support = true;
    cfg.print_speed_mm_s = 100.0;
    cfg.laser_power_W = 200.0;
    cfg.scan_speed_mm_s = 1000.0;
    cfg.hatch_spacing_mm = 0.1;
    return cfg;
}

/// @brief Write a simple ASCII STL from a surface mesh (triangle list)
inline bool writeSTL(const std::string& filepath,
                     const std::vector<float>& vertices,
                     const std::vector<uint32_t>& indices,
                     const std::string& solid_name = "UHTC_Part") {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "solid " << solid_name << "\n";
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        float ax = vertices[i0*3];
        float ay = vertices[i0*3+1];
        float az = vertices[i0*3+2];
        float bx = vertices[i1*3];
        float by = vertices[i1*3+1];
        float bz = vertices[i1*3+2];
        float cx = vertices[i2*3];
        float cy = vertices[i2*3+1];
        float cz = vertices[i2*3+2];

        float nx = (by - ay) * (cz - az) - (bz - az) * (cy - ay);
        float ny = (bz - az) * (cx - ax) - (bx - ax) * (cz - az);
        float nz = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
        float len = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (len > 1e-8f) { nx /= len; ny /= len; nz /= len; }

        out << "facet normal " << nx << " " << ny << " " << nz << "\n";
        out << "  outer loop\n";
        out << "    vertex " << ax << " " << ay << " " << az << "\n";
        out << "    vertex " << bx << " " << by << " " << bz << "\n";
        out << "    vertex " << cx << " " << cy << " " << cz << "\n";
        out << "  endloop\n";
        out << "endfacet\n";
    }
    out << "endsolid " << solid_name << "\n";
    out.close();
    return true;
}

/// @brief Write a simple CLI-like text slice file for LPBF/EBM printers
inline bool writeCLI(const std::string& filepath,
                     const std::vector<std::vector<float>>& slice_contours,
                     const SliceConfig& cfg) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "$$HEADERSTART\n";
    out << "$$VERSION " << 1.0 << "\n";
    out << "$$UNITS mm\n";
    out << "$$LAYERTHICK " << (cfg.layer_thickness_um / 1000.0) << "\n";
    out << "$$HEADEREND\n";

    for (size_t layer = 0; layer < slice_contours.size(); ++layer) {
        out << "\n$$LAYER " << layer << "\n";
        out << "$$CONTOUR\n";
        const auto& pts = slice_contours[layer];
        for (size_t i = 0; i < pts.size(); i += 2) {
            out << "$$LINE " << pts[i] << " " << pts[i+1] << "\n";
        }
        out << "$$ENDCONTOUR\n";
    }
    out.close();
    return true;
}

/// @brief High-level slicer: takes a grid and produces printer output files
class PrinterSlicer
{
public:
    PrinterSlicer(const std::string& printer_name, const std::string& material_name)
        : mPrinter(getPrinterProfile(printer_name))
        , mMaterial(getMaterialProperties(material_name))
        , mConfig(recommendSliceConfig(printer_name, material_name))
    {}

    const PrinterProfile& printer() const { return mPrinter; }
    const MaterialProperties& material() const { return mMaterial; }
    const SliceConfig& config() const { return mConfig; }

    /// @brief Export the part as STL
    bool exportSTL(const std::string& filepath,
                   const std::vector<float>& vertices,
                   const std::vector<uint32_t>& indices) const {
        return writeSTL(filepath, vertices, indices, mPrinter.name);
    }

    /// @brief Export slice data as CLI
    bool exportCLI(const std::string& filepath,
                   const std::vector<std::vector<float>>& slice_contours) const {
        return writeCLI(filepath, slice_contours, mConfig);
    }

    /// @brief Print configuration summary
    void printSummary() const {
        printf("Printer: %s (%s)\n", mPrinter.name.c_str(),
               mPrinter.family == PrinterFamily::LPBF ? "LPBF" :
               mPrinter.family == PrinterFamily::EBM ? "EBM" :
               mPrinter.family == PrinterFamily::DED ? "DED" : "Ceramic");
        printf("Material: %s\n", mMaterial.name.c_str());
        printf("Layer thickness: %.1f um\n", mConfig.layer_thickness_um);
        printf("Infill: %.1f%%\n", mConfig.infill_percent);
        printf("Supports: %s\n", mConfig.generate_supports ? "Yes" : "No");
        printf("Laser power: %.1f W\n", mConfig.laser_power_W);
        printf("Scan speed: %.1f mm/s\n", mConfig.scan_speed_mm_s);
    }

private:
    PrinterProfile mPrinter;
    MaterialProperties mMaterial;
    SliceConfig mConfig;
};

} // namespace tools
} // namespace nanovdb

#endif // NANOVDB_TOOLS_PRINTER_SLICER_OUTPUT_H_HAS_BEEN_INCLUDED
