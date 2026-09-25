# Telemetry

The startup experiment writes one CSV row per time step and a JSON summary.
Fields include valve command, actual valve, RPM, total flow, chamber pressure,
pump power, minimum NPSH and alarm codes. Future DAQ and FPGA adapters should
produce the same field names and SI units.