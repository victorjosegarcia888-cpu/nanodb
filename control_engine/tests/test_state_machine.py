import unittest

from control_engine.state_machine import StartupState, StartupStateMachine


class StartupStateMachineTest(unittest.TestCase):
    def test_nominal_simulation_sequence(self):
        machine = StartupStateMachine()
        for event in ("start", "purge_complete", "rpm_ready", "valves_ready",
                      "cold_flow_stable", "simulation_authorized", "shutdown", "stopped"):
            machine.dispatch(event)
        self.assertEqual(machine.state, StartupState.SAFE)
        self.assertEqual(len(machine.history), 9)

    def test_invalid_transition_is_rejected(self):
        machine = StartupStateMachine()
        with self.assertRaises(ValueError):
            machine.dispatch("rpm_ready")

    def test_fault_from_run_goes_to_shutdown(self):
        machine = StartupStateMachine()
        for event in ("start", "purge_complete", "rpm_ready", "valves_ready",
                      "cold_flow_stable", "simulation_authorized"):
            machine.dispatch(event)
        self.assertEqual(machine.dispatch("fault"), StartupState.SHUTDOWN)


if __name__ == "__main__":
    unittest.main()