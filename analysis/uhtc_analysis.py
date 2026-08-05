#!/usr/bin/env python3
"""
UHTC-NanoDB quantitative analysis for:
1. Transient thermal shock (0-500 ms)
2. Supercritical channel HTD risk
3. Aerospike nozzle + SWBLI

Run:
    python3 analysis/uhtc_analysis.py
"""

import math

# ============================================================
# 1. TRANSIENT THERMAL SHOCK
# ============================================================
def thermal_shock_analysis():
    print("=" * 60)
    print("TRANSIENT THERMAL SHOCK ANALYSIS (0-500 ms)")
    print("=" * 60)

    # UHTC material properties
    k_w = 80.0          # W/(m·K)
    rho_w = 6000.0      # kg/m³
    cp_w = 500.0        # J/(kg·K)
    alpha = k_w / (rho_w * cp_w)
    print(f"Thermal diffusivity alpha = {alpha:.4e} m²/s")

    # Thermal penetration depth
    times = [0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5]
    print("\nThermal penetration depth delta_th(t) = 2*sqrt(alpha*t):")
    for t in times:
        delta = 2.0 * math.sqrt(alpha * t)
        print(f"  t = {t*1000:4.0f} ms -> delta_th = {delta*1000:.3f} mm")

    # Wall parameters
    T_i = 300.0         # Initial temperature [K]
    T_aw = 3500.0       # Adiabatic wall temperature [K]
    h_g = 50000.0       # Gas-side HTC [W/(m²·K)]
    t_w = 0.020         # Wall thickness [m]

    print(f"\nWall: T_i={T_i} K, T_aw={T_aw} K, h_g={h_g} W/(m²·K), t_w={t_w*1000:.1f} mm")
    print("Semi-infinite assumption: delta_th(0.5s) < t_w ? ", end="")
    delta_05 = 2.0 * math.sqrt(alpha * 0.5)
    print(f"{delta_05*1000:.2f} mm < {t_w*1000:.1f} mm -> {delta_05 < t_w}")

    # Surface temperature T_gas(t)
    def T_gas(t):
        if t <= 0:
            return T_i
        beta = h_g * math.sqrt(alpha * t) / k_w
        rhs = 1.0 - math.exp(beta * beta) * math.erfc(beta)
        return T_i + (T_aw - T_i) * rhs

    print("\nSurface temperature T_gas(t):")
    for t in times:
        T = T_gas(t)
        print(f"  t = {t*1000:4.0f} ms -> T_gas = {T:.1f} K")

    # Stress analysis
    E = 380e9           # Pa
    nu = 0.18
    alpha_te = 5.5e-6   # 1/K

    print("\nTransient compressive stress sigma_th(t):")
    print(f"  E = {E:.2e} Pa, nu = {nu}, alpha_te = {alpha_te:.2e} 1/K")
    for t in times:
        T = T_gas(t)
        sigma = - (E * alpha_te * (T - T_i)) / (1.0 - nu)
        print(f"  t = {t*1000:4.0f} ms -> T={T:.1f} K -> sigma = {sigma/1e6:.1f} MPa")

    # Check yield
    sigma_yc = 800e6   # Pa (typical for UHTC at high T)
    t_yield = None
    for t in [0.001 * i for i in range(1, 501)]:
        T = T_gas(t)
        sigma = - (E * alpha_te * (T - T_i)) / (1.0 - nu)
        if abs(sigma) > sigma_yc:
            t_yield = t
            break
    if t_yield:
        print(f"\nYield reached at t = {t_yield*1000:.1f} ms")
    else:
        print("\nYield NOT reached in 0-500 ms window")


# ============================================================
# 2. SUPERCRITICAL CHANNEL HTD
# ============================================================
def supercritical_analysis():
    print("\n" + "=" * 60)
    print("SUPERCRITICAL COOLANT CHANNEL ANALYSIS")
    print("=" * 60)

    # Coolant state
    P = 2.0e6           # Pa
    T_bulk = 32.0       # K
    G = 200.0           # kg/(m²·s)
    q_flux = 1e6        # W/m²
    D_h = 1e-3          # m

    # Pseudocritical temperature (simplified)
    T_pc = 33.2         # K for H2
    Delta_T_crit = 5.0  # K

    print(f"Coolant: P={P/1e6:.1f} MPa, T_bulk={T_bulk:.1f} K, G={G:.0f} kg/(m²·s)")
    print(f"T_pc = {T_pc:.1f} K, Delta_T_crit = {Delta_T_crit:.1f} K")

    # HTD risk
    q_over_G = q_flux / G
    print(f"q_flux / G = {q_over_G:.2f} W·m/kg")
    if abs(T_bulk - T_pc) < Delta_T_crit:
        print("HTD RISK: T_bulk near pseudocritical temperature!")
    else:
        print("No immediate HTD risk.")

    # Nusselt correlation (simplified)
    rho_b = 40.0        # kg/m³
    mu_b = 1e-5         # Pa·s
    cp_b = 14200.0      # J/(kg·K)
    rho_w = 100.0       # kg/m³
    cp_mean = 8000.0    # J/(kg·K)
    k_w = 0.1           # W/(m·K)

    Re = G * D_h / mu_b
    Pr = mu_b * cp_b / k_w
    Nu = 0.021 * (Re ** 0.8) * (Pr ** 0.4) * ((rho_b / rho_w) ** 0.3) * ((cp_b / cp_mean) ** 1.0)
    print(f"\nRe = {Re:.2e}, Pr = {Pr:.2f}")
    print(f"Nu = {Nu:.2f}")
    h_c = Nu * k_w / D_h
    print(f"h_c = {h_c:.2f} W/(m²·K)")


# ============================================================
# 3. AEROSPIKE NOZZLE
# ============================================================
def aerospike_analysis():
    print("\n" + "=" * 60)
    print("AEROSPIKE NOZZLE + SWBLI ANALYSIS")
    print("=" * 60)

    gamma = 1.4
    M_exit = 3.5

    # Prandtl-Meyer
    def prandtl_meyer_nu(M):
        if M <= 1.0:
            return 0.0
        ratio = (gamma - 1.0) / (gamma + 1.0)
        term1 = math.sqrt((gamma + 1.0) / (gamma - 1.0))
        term2 = math.atan(math.sqrt(ratio * (M * M - 1.0)))
        term3 = math.atan(M * M - 1.0)
        return term1 * term2 - term3

    nu_exit = prandtl_meyer_nu(M_exit)
    print(f"Prandtl-Meyer angle at M={M_exit}: nu = {nu_exit*180/math.pi:.2f} deg")

    # Schmucker criterion
    M_sep = 2.5
    p_sep_over_p_a = (1.88 * (M_sep * M_sep - 1.0)) ** (-0.64)
    print(f"\nSchmucker separation ratio at M={M_sep}: P_sep/P_a = {p_sep_over_p_a:.4f}")

    # SWBLI lateral load
    separation_area = 1e-4  # m²
    ambient_p = 101325.0    # Pa
    lateral_load = separation_area * ambient_p * (1.0 - p_sep_over_p_a)
    print(f"SWBLI lateral load (area={separation_area:.2e} m²): {lateral_load:.2f} N")


if __name__ == "__main__":
    thermal_shock_analysis()
    supercritical_analysis()
    aerospike_analysis()
