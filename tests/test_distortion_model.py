import json
import math
import tempfile
import unittest
from pathlib import Path

from distortion_model import compute_calibration, export_calibration, update_from_sensor


class CalibrationTests(unittest.TestCase):
    def setUp(self):
        self.base = dict(k1=.1, k2=.01, k3=.001, p1=.002, p2=.003,
                         fx=2, fy=3, cx=.5, cy=.25)

    def test_projection_and_coefficients(self):
        result = compute_calibration(self.base)
        matrix = [result["projection_matrix"][i:i + 4] for i in range(0, 16, 4)]
        self.assertEqual(matrix[:3], [[2, 0, .5, 0], [0, 3, .25, 0], [0, 0, 1, 0]])
        self.assertEqual(result["k1"], .1)

    def test_extrinsics_and_sensor_update(self):
        base = {**self.base, "t": [1, 2, 3], "timestamp": 4}
        result = update_from_sensor(base, {"eye_offset_x": .25, "eye_offset_y": -.25,
                                           "timestamp": 5})
        matrix = [result["projection_matrix"][i:i + 4] for i in range(0, 16, 4)]
        self.assertAlmostEqual(matrix[0][2], .75)
        self.assertEqual(result["timestamp"], 5)

    def test_export_and_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nested" / "calibration.json"
            export_calibration(compute_calibration(self.base), path)
            self.assertEqual(json.loads(path.read_text())["p2"], .003)
            self.assertEqual(list(path.parent.iterdir()), [path])
        with self.assertRaisesRegex(ValueError, "missing"):
            compute_calibration({})

    def test_timestamp_validation(self):
        for invalid in (-1, 1.0, 1.5, float("inf"), True, "1", 2**64):
            with self.subTest(timestamp=invalid):
                with self.assertRaisesRegex(ValueError, "timestamp"):
                    compute_calibration({**self.base, "timestamp": invalid})
        self.assertEqual(compute_calibration({**self.base, "timestamp": 2**64 - 1})["timestamp"],
                         2**64 - 1)

    def test_numeric_domain_and_shape_validation(self):
        for field, value in (("k1", float("nan")), ("fx", float("inf")),
                             ("p1", True), ("cy", "0.25"), ("k2", 4e38)):
            with self.subTest(field=field, value=value):
                with self.assertRaisesRegex(ValueError, field):
                    compute_calibration({**self.base, field: value})
        for field, value in (("R", None), ("R", [1, 2, 3]),
                             ("t", None), ("t", [[1], [2]])):
            with self.subTest(field=field, value=value):
                with self.assertRaisesRegex(ValueError, field):
                    compute_calibration({**self.base, field: value})
        with self.assertRaisesRegex(ValueError, "projection_matrix"):
            compute_calibration({**self.base, "fx": 3e38, "t": [2, 0, 0]})

    def test_sensor_update_rejects_invalid_evidence_without_mutating_base(self):
        base = {**self.base, "timestamp": 7}
        original = dict(base)
        for sensor in (
                {"eye_offset_x": float("nan"), "eye_offset_y": 0},
                {"eye_offset_x": 0, "eye_offset_y": float("inf")},
                {"eye_offset_x": "0", "eye_offset_y": 0},
                {"eye_offset_x": False, "eye_offset_y": 0},
                {"eye_offset_x": 0, "eye_offset_y": 0, "timestamp": 1.5},
                {"eye_offset_x": 0, "eye_offset_y": 0, "timestamp": -1}):
            with self.subTest(sensor=sensor):
                with self.assertRaises((ValueError, KeyError)):
                    update_from_sensor(base, sensor)
                self.assertEqual(base, original)

    def test_export_rejects_malformed_data_and_preserves_destination(self):
        valid = compute_calibration(self.base)
        malformed_documents = [
            {**valid, "k1": math.nan},
            {**valid, "projection_matrix": valid["projection_matrix"][:-1]},
            {**valid, "timestamp": 1.0},
            {key: value for key, value in valid.items() if key != "p2"},
        ]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "calibration.json"
            path.write_text("known-good", encoding="utf-8")
            for malformed in malformed_documents:
                with self.subTest(malformed=malformed):
                    with self.assertRaises(ValueError):
                        export_calibration(malformed, path)
                    self.assertEqual(path.read_text(encoding="utf-8"), "known-good")
                    self.assertEqual(list(path.parent.iterdir()), [path])


if __name__ == "__main__":
    unittest.main()
