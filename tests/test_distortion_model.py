import json
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
        for invalid in (-1, 1.5, float("inf"), 2**64):
            with self.subTest(timestamp=invalid):
                with self.assertRaisesRegex(ValueError, "timestamp"):
                    compute_calibration({**self.base, "timestamp": invalid})
        self.assertEqual(compute_calibration({**self.base, "timestamp": 2**64 - 1})["timestamp"],
                         2**64 - 1)


if __name__ == "__main__":
    unittest.main()
