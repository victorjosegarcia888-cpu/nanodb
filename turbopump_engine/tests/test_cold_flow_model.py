import json
import unittest
from pathlib import Path

from turbopump_engine.models.cold_flow_model import analyze


class ColdFlowModelTest(unittest.TestCase):
    def test_scaled_case_has_positive_npsh_and_pressure_budget(self):
        case_path = Path(__file__).parents[1] / "cases" / "lox_ch4_cold_flow_scaled.json"
        report = analyze(json.loads(case_path.read_text(encoding="utf-8")))
        self.assertTrue(report["checks"]["mass_balance"])
        self.assertTrue(report["checks"]["positive_pressure_budget"])
        self.assertTrue(report["checks"]["positive_npsh_margin"])
        self.assertTrue(report["checks"]["property_source_review_required"])
        self.assertFalse(report["checks"]["properties_validated"])
        self.assertTrue(all(result["shaft_power_w"] > 0.0 for result in report["results"]))


if __name__ == "__main__":
    unittest.main()