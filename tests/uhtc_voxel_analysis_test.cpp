#include <cstdio>
#include <cmath>
#include <cassert>

#include "PicoGKApiTypes.h"
#include "nanodb/nanovdb/tools/TransientThermalShock.h"
#include "nanodb/nanovdb/tools/SupercriticalChannel.h"
#include "nanodb/nanovdb/tools/AerospikeNozzle.h"

using namespace nanovdb::tools;

int main() {
    printf("=== UHTC Thermal/Fluid/Nozzle Voxel Tests ===\n");

    // ---- Thermal Shock ----
    printf("[1] Transient thermal shock... ");
    {
        UHTCThermalShockParams tp;
        tp.T_i = 300.0;
        tp.T_aw = 3500.0;
        tp.k_w = 80.0;
        tp.rho_w = 6000.0;
        tp.cp_w = 500.0;
        tp.h_g = 50000.0;
        tp.t_w = 0.02;
        tp.t_max = 0.5;

        double alpha = computeThermalDiffusivity(tp.k_w, tp.rho_w, tp.cp_w);
        double delta_05 = computeThermalPenetrationDepth(alpha, 0.5);

        if (std::fabs(alpha - 2.6667e-5) > 1e-7) {
            printf("FAIL alpha=%e\n", alpha);
            return 1;
        }
        if (delta_05 < 0.006 || delta_05 > 0.0085) {
            printf("FAIL delta_05=%f\n", delta_05);
            return 1;
        }

        double T0 = computeSurfaceTemperature(0.0, tp);
        double T05 = computeSurfaceTemperature(0.5, tp);
        if (T0 != tp.T_i || T05 <= tp.T_i || T05 >= tp.T_aw) {
            printf("FAIL T0=%f T05=%f\n", T0, T05);
            return 1;
        }

        UHTCStressParams sp;
        sp.E = 380e9;
        sp.nu = 0.18;
        sp.alpha_te = 5.5e-6;

        double sigma = computeTransientCompressiveStress(0.5, tp, sp);
        if (sigma > 0.0) {
            printf("FAIL sigma=%f (expected negative compression)\n", sigma);
            return 1;
        }

        TransientThermalShockVoxels<float> vox(tp, sp, 0.5, 32, 32, 32);
        auto grid = vox.buildGrid(0.5);
        if (!grid) {
            printf("FAIL grid null\n");
            return 1;
        }
        printf("OK\n");
    }

    // ---- Supercritical Channel ----
    printf("[2] Supercritical channel / HTD... ");
    {
        SupercriticalCoolantState state;
        state.P = 2.0e6;
        state.T_bulk = 32.0;
        state.G = 200.0;
        state.q_flux = 1e6;
        state.D_h = 1e-3;

        double T_pc = getPseudocriticalTemperature(state);
        if (T_pc <= 0.0) {
            printf("FAIL T_pc\n");
            return 1;
        }

        double Nu = computeSupercriticalNusselt(state, 40.0, 1e-5, 14200.0, 100.0, 8000.0, 0.1);
        if (!std::isfinite(Nu) || Nu <= 0.0) {
            printf("FAIL Nu=%f\n", Nu);
            return 1;
        }

        MicroMappedChannelSDF sdf(state, 1.0e-3, T_pc, 5.0, 0.15f);
        PKVector3 p = {0.0f, 0.0f, 0.0f};
        float f = sdf(p);
        if (!std::isfinite(f)) {
            printf("FAIL SDF\n");
            return 1;
        }
        printf("OK\n");
    }

    // ---- Aerospike / SWBLI ----
    printf("[3] Aerospike nozzle / SWBLI... ");
    {
        double nu_exit = prandtlMeyerNu(3.5);
        if (!std::isfinite(nu_exit) || nu_exit <= 0.0) {
            printf("FAIL PM\n");
            return 1;
        }

        double M_inv = invertPrandtlMeyer(nu_exit);
        if (std::fabs(M_inv - 3.5) > 0.1) {
            printf("FAIL PM inv M=%f\n", M_inv);
            return 1;
        }

        AerospikePlugSDF nozzle(0.02, nu_exit, 0.15, 2.0e-4, 0.006);
        PKVector3 p = {0.0f, 0.0f, 0.05f};
        float f = nozzle(p);
        if (!std::isfinite(f)) {
            printf("FAIL SDF\n");
            return 1;
        }

        double ratio = AerospikePlugSDF::schmuckerSeparationRatio(2.5);
        if (ratio <= 0.0 || ratio > 1.0) {
            printf("FAIL schmucker ratio=%f\n", ratio);
            return 1;
        }

        float lateral = computeSWBLILateralLoad(1e-4, ratio, 101325.0);
        if (!std::isfinite(lateral) || lateral < 0.0) {
            printf("FAIL SWBLI load\n");
            return 1;
        }
        printf("OK\n");
    }

    printf("\nAll UHTC voxel analysis tests passed.\n");
    return 0;
}
