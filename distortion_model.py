"""Calibration model shared with the native optical compensation layer."""

from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Any, Mapping

REQUIRED_FIELDS = ("k1", "k2", "k3", "p1", "p2", "fx", "fy", "cx", "cy")


def compute_calibration(calibration: Mapping[str, Any]) -> dict[str, Any]:
    """Validate calibration input and produce shader-ready coefficients and matrix."""
    missing = [key for key in REQUIRED_FIELDS if key not in calibration]
    if missing:
        raise ValueError(f"missing calibration field(s): {', '.join(missing)}")

    values = {key: float(calibration[key]) for key in REQUIRED_FIELDS}
    if not all(math.isfinite(value) for value in values.values()):
        raise ValueError("calibration values must be finite")
    if values["fx"] <= 0 or values["fy"] <= 0:
        raise ValueError("fx and fy must be positive")

    intrinsic = [[values["fx"], 0.0, values["cx"]],
                 [0.0, values["fy"], values["cy"]], [0.0, 0.0, 1.0]]
    rotation = calibration.get("R", [[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    translation = calibration.get("t", [[0], [0], [0]])
    if len(rotation) != 3 or any(len(row) != 3 for row in rotation):
        raise ValueError("R must have shape (3, 3)")
    if len(translation) == 3 and all(not isinstance(item, (list, tuple)) for item in translation):
        translation = [[item] for item in translation]
    if len(translation) != 3 or any(len(row) != 1 for row in translation):
        raise ValueError("t must have shape (3, 1)")
    rotation = [[float(value) for value in row] for row in rotation]
    translation = [[float(value) for value in row] for row in translation]
    if not all(math.isfinite(value) for row in rotation + translation for value in row):
        raise ValueError("R and t must contain finite values")

    # Form K[R|t] explicitly instead of depending on NumPy. Keeping this module
    # dependency-free makes calibration generation usable in constrained tooling.
    extrinsic = [rotation[row] + translation[row] for row in range(3)]
    projected = [[sum(intrinsic[row][k] * extrinsic[k][column] for k in range(3))
                  for column in range(4)] for row in range(3)]
    projection = projected + [[0.0, 0.0, 0.0, 1.0]]
    result: dict[str, Any] = {
        key: values[key] for key in ("k1", "k2", "k3", "p1", "p2")
    }
    # C order is explicit; the C++ bridge uploads with transpose=true.
    result["projection_matrix"] = [value for row in projection for value in row]
    timestamp = calibration.get("timestamp", 0)
    try:
        integer_timestamp = int(timestamp)
        numeric_timestamp = float(timestamp)
    except (TypeError, ValueError, OverflowError) as error:
        raise ValueError("timestamp must be a non-negative integer") from error
    if (isinstance(timestamp, bool) or not math.isfinite(numeric_timestamp) or
            numeric_timestamp < 0 or not numeric_timestamp.is_integer() or
            integer_timestamp > 2**64 - 1):
        raise ValueError("timestamp must be a non-negative uint64 integer")
    result["timestamp"] = integer_timestamp
    return result


def export_calibration(params: Mapping[str, Any], path: str | Path) -> None:
    """Atomically export calibration parameters as JSON."""
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    # replace() prevents readers from observing a partially written calibration.
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    temporary.write_text(json.dumps(dict(params), indent=2) + "\n", encoding="utf-8")
    temporary.replace(destination)


def update_from_sensor(base: Mapping[str, Any], sensor: Mapping[str, Any]) -> dict[str, Any]:
    """Recompute calibration after applying the current eye displacement."""
    updated = dict(base)
    updated["cx"] = float(updated["cx"]) + float(sensor["eye_offset_x"])
    updated["cy"] = float(updated["cy"]) + float(sensor["eye_offset_y"])
    if "timestamp" in sensor:
        updated["timestamp"] = int(sensor["timestamp"])
    return compute_calibration(updated)
