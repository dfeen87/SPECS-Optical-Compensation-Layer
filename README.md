# Specs Optical Compensation Layer

Version 1 is a small, testable reference implementation of an optical correction
pipeline. Python validates device/user calibration and exports a JSON interchange
file; C++ loads that file, binds its values, and runs the sensor-driven render loop.
The standalone and embedded GLSL shaders apply radial and tangential correction.

## Calibration

```python
from distortion_model import compute_calibration, export_calibration

calibration = compute_calibration({
    "k1": -0.12, "k2": 0.03, "k3": 0.0, "p1": 0.0, "p2": 0.0,
    "fx": 1.0, "fy": 1.0, "cx": 0.0, "cy": 0.0,
    "timestamp": 1,
})
export_calibration(calibration, "calibration.json")
```

`R` (3×3) and `t` (3×1 or a three-value vector) are optional. The output matrix is
row-major JSON. The bridge deliberately requests a transpose during upload so that
it has the expected meaning in OpenGL.

## Native integration

The library does not impose a windowing library or GL loader. Implement
`specs::GraphicsApi` using the application's OpenGL functions and populate
`specs::PipelineCallbacks` using the device SDK. Shader compile and link failures
should be reported by the `GraphicsApi` implementation as exceptions; the bridge
then cleans up intermediate shader objects.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The full shaders are in `shaders/`; equivalent embedded sources are exported as
`correctiveVertexShaderSource` and `correctiveFragmentShaderSource`. Texture UVs are
converted to centered normalized coordinates before correction and converted back
before sampling. Applications should use a clamp-to-border texture mode when pixels
outside the corrected image should be black.

## Python tests

```bash
python -m unittest discover -s tests -p 'test_*.py'
```
