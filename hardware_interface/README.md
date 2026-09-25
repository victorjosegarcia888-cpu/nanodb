# Hardware interface

This is the boundary for future DAQ, FPGA, encoder and pressure-transducer
adapters. The simulation must remain usable without hardware. Any live adapter
must expose timestamps, units, validity flags and a fail-closed behavior for
missing or out-of-range signals.