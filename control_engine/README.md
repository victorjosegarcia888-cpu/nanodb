# Control engine

The first experiment is a deterministic startup sequence. It ramps a valve,
spins the pump with a first-order response, propagates flow through the lines
and injector, and relaxes a non-reacting chamber pressure state.

The startup state machine is:

```text
SAFE
 -> PURGE
 -> SPIN
 -> VALVES_CHECK
 -> COLD_FLOW
 -> IGNITION_AUTHORIZED_SIMULATION
 -> RUN
 -> SHUTDOWN
 -> SAFE
```

`IGNITION_AUTHORIZED_SIMULATION` is an explicit simulation-only state. It does
not authorize ignition hardware or a pressurized test.

Safety checks stop the virtual valve on low NPSH, excessive chamber pressure or
overspeed. This is a software experiment only; it is not an ignition or hot-fire
controller.

Run it with:

```bash
python3 -m unittest discover -s control_engine/tests -v
python3 control_engine/simulation.py \
  turbopump_engine/cases/cold_startup_experiment.json \
  --output-directory turbopump_engine/cases/cold_startup_experiment_report
```