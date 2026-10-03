"""Calibration model shared with the native optical compensation layer."""

from __future__ import annotations

import json
import math
import os
from pathlib import Path
import tempfile
from typing import Any, Mapping

REQUIRED_FIELDS = ("k1", "k2", "k3", "p1", "p2", "fx", "fy", "cx", "cy")
SHADER_FIELDS = ("k1", "k2", "k3", "p1", "p2")
FLOAT32_MAX = 3.4028234663852886e38


def _finite_float(value: Any, name: str) -> float:
    """Return a finite, native-compatible float without coercing non-numbers."""
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{name} must be a number")
    converted = float(value)
    if not math.isfinite(converted):
        raise ValueError(f"{name} must be finite")
    if abs(converted) > FLOAT32_MAX:
        raise ValueError(f"{name} is outside the native float range")
    return converted


def _uint64(value: Any, name: str = "timestamp") -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 2**64 - 1:
        raise ValueError(f"{name} must be a non-negative uint64 integer")
    return value


def compute_calibration(calibration: Mapping[str, Any]) -> dict[str, Any]:
    """Validate calibration input and produce shader-ready coefficients and matrix."""
    missing = [key for key in REQUIRED_FIELDS if key not in calibration]
    if missing:
        raise ValueError(f"missing calibration field(s): {', '.join(missing)}")

    values = {key: _finite_float(calibration[key], key) for key in REQUIRED_FIELDS}
    if values["fx"] <= 0 or values["fy"] <= 0:
        raise ValueError("fx and fy must be positive")

    intrinsic = [[values["fx"], 0.0, values["cx"]],
                 [0.0, values["fy"], values["cy"]], [0.0, 0.0, 1.0]]
    rotation = calibration.get("R", [[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    translation = calibration.get("t", [[0], [0], [0]])
    if (not isinstance(rotation, (list, tuple)) or len(rotation) != 3 or
            any(not isinstance(row, (list, tuple)) or len(row) != 3 for row in rotation)):
        raise ValueError("R must have shape (3, 3)")
    if (isinstance(translation, (list, tuple)) and len(translation) == 3 and
            all(not isinstance(item, (list, tuple)) for item in translation)):
        translation = [[item] for item in translation]
    if (not isinstance(translation, (list, tuple)) or len(translation) != 3 or
            any(not isinstance(row, (list, tuple)) or len(row) != 1 for row in translation)):
        raise ValueError("t must have shape (3, 1)")
    rotation = [[_finite_float(value, "R") for value in row] for row in rotation]
    translation = [[_finite_float(value, "t") for value in row] for row in translation]

    # Form K[R|t] explicitly instead of depending on NumPy. Keeping this module
    # dependency-free makes calibration generation usable in constrained tooling.
    extrinsic = [rotation[row] + translation[row] for row in range(3)]
    projected = [[sum(intrinsic[row][k] * extrinsic[k][column] for k in range(3))
                  for column in range(4)] for row in range(3)]
    if not all(math.isfinite(value) and abs(value) <= FLOAT32_MAX
               for row in projected for value in row):
        raise ValueError("projection_matrix is outside the native float range")
    projection = projected + [[0.0, 0.0, 0.0, 1.0]]
    result: dict[str, Any] = {
        key: values[key] for key in ("k1", "k2", "k3", "p1", "p2")
    }
    # C order is explicit; the C++ bridge uploads with transpose=true.
    result["projection_matrix"] = [value for row in projection for value in row]
    result["timestamp"] = _uint64(calibration.get("timestamp", 0))
    return result


def _validated_export(params: Mapping[str, Any]) -> dict[str, Any]:
    """Validate the complete Python-to-native interchange contract."""
    missing = [key for key in (*SHADER_FIELDS, "projection_matrix", "timestamp")
               if key not in params]
    if missing:
        raise ValueError(f"missing export field(s): {', '.join(missing)}")
    result = {key: _finite_float(params[key], key) for key in SHADER_FIELDS}
    matrix = params["projection_matrix"]
    if not isinstance(matrix, (list, tuple)) or len(matrix) != 16:
        raise ValueError("projection_matrix must contain 16 numbers")
    result["projection_matrix"] = [
        _finite_float(value, "projection_matrix") for value in matrix
    ]
    result["timestamp"] = _uint64(params["timestamp"])
    return result


def export_calibration(params: Mapping[str, Any], path: str | Path) -> None:
    """Atomically export calibration parameters as JSON."""
    validated = _validated_export(params)
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    # A unique file in the destination directory makes concurrent exports safe;
    # replace() then prevents readers from observing a partially written file.
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", dir=destination.parent,
                prefix=f".{destination.name}.", suffix=".tmp", delete=False) as temporary:
            temporary_name = temporary.name
            json.dump(validated, temporary, indent=2, allow_nan=False)
            temporary.write("\n")
            temporary.flush()
            os.fsync(temporary.fileno())
        Path(temporary_name).replace(destination)
    finally:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)


def update_from_sensor(base: Mapping[str, Any], sensor: Mapping[str, Any]) -> dict[str, Any]:
    """Recompute calibration after applying the current eye displacement."""
    updated = dict(base)
    updated["cx"] = (_finite_float(updated["cx"], "cx") +
                     _finite_float(sensor["eye_offset_x"], "eye_offset_x"))
    updated["cy"] = (_finite_float(updated["cy"], "cy") +
                     _finite_float(sensor["eye_offset_y"], "eye_offset_y"))
    if "timestamp" in sensor:
        updated["timestamp"] = sensor["timestamp"]
    return compute_calibration(updated)
