// ============================================================
// Program.cs — Simulación comparativa de rendimiento NASA / SpaceX
// ============================================================

using System;
using RocketPropulsion.Data;
using RocketPropulsion.Tasks;

namespace RocketPropulsion
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("=== Simulación Comparativa NASA / SpaceX ===\n");

            // --- Volumen de cámara Raptor ---
            double Vc = TasksRaptor.CalcularVolumenCamara();
            Console.WriteLine($"Volumen cámara Raptor: {Vc:F3} m³");

            // --- Relación de expansión ---
            double AeAt = TasksRaptor.CalcularRelacionExpansion();
            Console.WriteLine($"Relación Ae/At: {AeAt:F2}");

            // --- Potencia de turbina FFSCC ---
            double Potencia = TasksRaptor.CalcularPotenciaTurbina(120.0, 2800.0, 950.0, 1.5, 0.85);
            Console.WriteLine($"Potencia turbina FFSCC: {Potencia:F2} MW");

            // --- Ángulo de spray Pintle ---
            double Angulo = TasksRaptor.CalcularAnguloSpray(1.0);
            Console.WriteLine($"Ángulo de spray Pintle: {Angulo:F1}°");

            // --- Eficiencia térmica ---
            double Eficiencia = TasksRaptor.CalcularEficienciaTermica(DatosMotor.Isp_Raptor, 5e7);
            Console.WriteLine($"Eficiencia térmica Raptor: {Eficiencia:P2}");

            Console.WriteLine("\nSimulación completada con parámetros NASA / SpaceX.");
        }
    }
}
