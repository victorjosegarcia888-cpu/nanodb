import json
import unittest
from pathlib import Path

from control_engine.simulation import simulate


class StartupSimulationTest(unittest.TestCase):
    def test_valve_startup_reaches_cold_chamber_without_alarm(self):
        case_path = Path(__file__).parents[2] / "turbopump_engine" / "cases" / "cold_startup_experiment.json"
        rows = simulate(json.loads(case_path.read_text(encoding="utf-8")))
        self.assertGreater(len(rows), 100)
        self.assertEqual(rows[0]["valve"], 0.0)
        self.assertGreater(rows[-1]["valve"], 0.9)
        self.assertGreater(rows[-1]["total_flow_kg_s"], 0.0)
        self.assertGreater(rows[-1]["chamber_pressure_pa"], rows[0]["chamber_pressure_pa"])
        self.assertFalse(any(row["alarms"] for row in rows))


if __name__ == "__main__":
    unittest.main()