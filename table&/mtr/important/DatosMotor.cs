// ============================================================
// DatosMotor.cs — Parámetros NASA / SpaceX para motores LOX/CH4
// ============================================================

using System;
namespace RocketPropulsion.Data
{
    public static class DatosMotor
    {
        // --- RAPTOR ---
        public static readonly double Pc_Raptor = 2.8e7;          // Pa
        public static readonly double Tc_Raptor = 3500.0;         // K
        public static readonly double OF_Raptor = 3.6;
        public static readonly double Isp_Raptor = 380.0;         // s
        public static readonly double Empuje_Raptor = 2.0e6;      // N
        public static readonly double Masa_Raptor = 1800.0;       // kg
        public static readonly double Dt_Raptor = 0.4;            // m
        public static readonly double De_Raptor = 1.3;            // m
        public static readonly double L_Raptor = 0.48;            // m
        public static readonly double HeatFlux_Raptor = 1.5e7;    // W/m²

        // --- MERLIN ---
        public static readonly double Pc_Merlin = 9.7e6;
        public static readonly double Tc_Merlin = 3600.0;
        public static readonly double OF_Merlin = 2.7;
        public static readonly double Isp_Merlin = 311.0;
        public static readonly double Empuje_Merlin = 8.45e5;
        public static readonly double Masa_Merlin = 470.0;
        public static readonly double Dt_Merlin = 0.27;
        public static readonly double De_Merlin = 1.0;
        public static readonly double L_Merlin = 0.35;

        // --- RS-25 ---
        public static readonly double Pc_RS25 = 2.07e7;
        public static readonly double Tc_RS25 = 3600.0;
        public static readonly double OF_RS25 = 6.0;
        public static readonly double Isp_RS25 = 452.0;
        public static readonly double Empuje_RS25 = 1.86e6;
        public static readonly double Masa_RS25 = 3170.0;

        // --- PREQUEMADORES FFSCC ---
        public static readonly double Tpb_Min = 750.0;
        public static readonly double Tpb_Max = 1000.0;
        public static readonly double OF_ORPB_Min = 20.0;
        public static readonly double OF_ORPB_Max = 40.0;
        public static readonly double OF_FRPB_Min = 0.2;
        public static readonly double OF_FRPB_Max = 0.4;
        public static readonly double Potencia_Turbina = 15.0; // MW

        // --- INYECTOR PINTLE ---
        public static readonly double TMR_Min = 0.5;
        public static readonly double TMR_Max = 1.5;
        public static readonly double SprayAngle_Min = 40.0;
        public static readonly double SprayAngle_Max = 60.0;
        public static readonly double SMD_Min = 40.0; // µm
        public static readonly double SMD_Max = 80.0; // µm

        // --- TURBINA ALTA PRESIÓN ---
        public static readonly double OTDF_Max = 0.22;
        public static readonly double Tmax_Turbina = 1000.0;
        public static readonly double Gradiente_Termico = 1500.0; // K/s
    }
}
