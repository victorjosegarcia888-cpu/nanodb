// ============================================================
// TasksRaptor.cs — Tareas de simulación para cámara, tobera,
// inyección y ciclo FFSCC (NASA / SpaceX)
// ============================================================

using System;
using RocketPropulsion.Data;

namespace RocketPropulsion.Tasks
{
    public static class TasksRaptor
    {
        // --- CÁMARA DE COMBUSTIÓN ---
        public static double CalcularVolumenCamara()
        {
            double Vc = Math.PI * Math.Pow(DatosMotor.Dt_Raptor / 2.0, 2.0) * DatosMotor.L_Raptor;
            return Vc;
        }

        // --- TOBERA ---
        public static double CalcularRelacionExpansion()
        {
            double Ae_At = Math.Pow(DatosMotor.De_Raptor / DatosMotor.Dt_Raptor, 2.0);
            return Ae_At;
        }

        // --- INYECTOR PINTLE ---
        public static double CalcularAnguloSpray(double TMR)
        {
            double cosTheta = 1.0 / (1.0 + TMR);
            return Math.Acos(cosTheta) * (180.0 / Math.PI);
        }

        // --- CICLO FFSCC ---
        public static double CalcularPotenciaTurbina(double mdot, double Cp, double Tpb, double PR, double eta)
        {
            double gamma = 1.3;
            double tempDropFactor = 1.0 - Math.Pow(1.0 / PR, (gamma - 1.0) / gamma);
            double W = (mdot * Cp * Tpb * tempDropFactor * eta) / 1e6; // MW
            return W;
        }

        // --- EFICIENCIA GLOBAL ---
        public static double CalcularEficienciaTermica(double Isp, double LHV)
        {
            double ve = Isp * 9.80665;
            return (0.5 * Math.Pow(ve, 2.0)) / LHV;
        }
    }
}
