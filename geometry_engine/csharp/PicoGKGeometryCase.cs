using System;
using System.Globalization;
using System.IO;

namespace GeometryEngine
{
    // Data-only bridge: the PicoGKRuntime-specific voxel calls belong in the
    // host application so this contract stays independent of runtime version.
    public sealed class PicoGKGeometryCase
    {
        public string CaseId { get; }
        public double ThroatRadiusMeters { get; }
        public double ExitRadiusMeters { get; }
        public double NozzleLengthMeters { get; }
        public int Samples { get; }

        public PicoGKGeometryCase(string caseId, double throatRadiusMeters,
            double exitRadiusMeters, double nozzleLengthMeters, int samples = 256)
        {
            if (string.IsNullOrWhiteSpace(caseId)) throw new ArgumentException("caseId");
            if (throatRadiusMeters <= 0 || exitRadiusMeters <= throatRadiusMeters ||
                nozzleLengthMeters <= 0 || samples < 2) throw new ArgumentOutOfRangeException();
            CaseId = caseId;
            ThroatRadiusMeters = throatRadiusMeters;
            ExitRadiusMeters = exitRadiusMeters;
            NozzleLengthMeters = nozzleLengthMeters;
            Samples = samples;
        }

        public void WriteProfileCsv(string path)
        {
            using var writer = new StreamWriter(path);
            writer.WriteLine("z_m,radius_m");
            for (int index = 0; index < Samples; index++)
            {
                double t = (double)index / (Samples - 1);
                double smooth = 10.0 * Math.Pow(t, 3) - 15.0 * Math.Pow(t, 4) + 6.0 * Math.Pow(t, 5);
                double radius = ThroatRadiusMeters + (ExitRadiusMeters - ThroatRadiusMeters) * smooth;
                writer.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0},{1}",
                    NozzleLengthMeters * t, radius));
            }
        }
    }
}