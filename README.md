# SPECS Optical Compensation Layer

[![CI](https://github.com/dfeen87/SPECS-Optical-Compensation-Layer/actions/workflows/ci.yml/badge.svg)](https://github.com/dfeen87/SPECS-Optical-Compensation-Layer/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](CMakeLists.txt)
[![Python 3](https://img.shields.io/badge/Python-3-3776AB.svg)](distortion_model.py)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](CHANGELOG.md)

## About

SPECS is a small, dependency-light reference implementation of an optical
compensation pipeline. It turns device and user calibration data into a
shader-ready interchange file, loads those values in a native application, and
applies Brown&ndash;Conrady radial and tangential correction on the GPU.

The project intentionally does **not** prescribe a device SDK, windowing system,
or OpenGL loader. Instead, the C++ layer exposes narrow graphics and pipeline
interfaces that an application can adapt to its own runtime.

> [!NOTE]
> This repository is a reference implementation rather than a complete end-user application. The host application remains responsible for creating the OpenGL context, supplying frames and sensor data, and presenting rendered output. Beyond vision correction, the underlying software-defined calibration layer is designed to explore a broader possibility: dynamically correcting, enhancing, filtering, or spatially transforming the wearer’s visual environment in real time. 

## Highlights

- Dependency-free Python calibration and atomic JSON export.
- C++17 calibration loading and a callback-driven rendering loop.
- Standalone and embedded GLSL 3.30 corrective shaders.
- Per-frame eye-offset updates with calibration refreshes only when required.
- No required runtime framework, JSON library, or Python package.
- Automated native and Python tests through GitHub Actions.

## Pipeline overview

```text
Device calibration + eye pose
            |
            v
  distortion_model.py  ----->  calibration.json
                                      |
                                      v
                         C++ compensation bridge
                         (host-provided callbacks)
                                      |
                                      v
                       GLSL correction + presentation
```

Python validates the lens coefficients and camera parameters, forms a row-major
4&times;4 projection matrix, and exports JSON. C++ loads that interchange file,
uploads its static values, and updates dynamic eye offsets during the render loop.
The shaders perform inverse radial and tangential correction in centered lens
space before sampling the input frame.

For the detailed design, mathematics, parameter mapping, and glossary, see
[`PAPER.md`](PAPER.md).

## Requirements

| Component | Requirement |
| --- | --- |
| Calibration tooling | Python 3 (standard library only) |
| Native library | CMake 3.16+ and a C++17 compiler |
| Shader runtime | An OpenGL environment supporting GLSL 3.30 core |
| Application integration | Implementations of `specs::GraphicsApi` and `specs::PipelineCallbacks` |

## Quick start

### 1. Build and test the native library

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

This creates the `optical_compensation` library and, when testing is enabled,
the `optical_compensation_test` executable.

To install the C++ library and consume its exported CMake package:

```bash
cmake --install build --prefix /your/install/prefix
```

Downstream CMake projects can then use
`find_package(SPECSOpticalCompensation 1 CONFIG REQUIRED)` and link the
`SPECS::optical_compensation` target. The public header also exposes
`specs::versionString` for runtime diagnostics.

### 2. Run the Python tests

```bash
python -m unittest discover -s tests -p 'test_*.py'
```

### 3. Generate a calibration file

```python
from distortion_model import compute_calibration, export_calibration

calibration = compute_calibration({
    "k1": -0.12,
    "k2": 0.03,
    "k3": 0.0,
    "p1": 0.0,
    "p2": 0.0,
    "fx": 1.0,
    "fy": 1.0,
    "cx": 0.0,
    "cy": 0.0,
    "timestamp": 1,
})

export_calibration(calibration, "calibration.json")
```

`R` (3&times;3) and `t` (3&times;1 or a three-value vector) are optional and
default to an identity rotation and zero translation. `timestamp` is optional and
defaults to `0`; when supplied, it must be an unsigned 64-bit integer.

### 4. Connect the native runtime

Include the public header and load the exported values:

```cpp
#include "optical_compensation.hpp"

specs::CalibrationParams calibration =
    specs::loadParamsFromJson("calibration.json");
```

Then:

1. Implement `specs::GraphicsApi` with the host application's OpenGL calls.
2. Create a program with `createCorrectiveShaderProgram`, using either the files
   in `shaders/` or the embedded `corrective*ShaderSource` strings.
3. Populate `specs::PipelineCallbacks` from the device SDK and presentation loop.
4. Call `runCorrectivePipeline`; it runs until `deviceIsRunning` returns `false`.

Shader compilation and linking errors should be surfaced as exceptions by the
`GraphicsApi` implementation. The bridge cleans up intermediate shader objects,
while ownership of the linked program stays with the caller.

## Calibration interface

The Python input uses the following values:

| Field | Meaning | Constraint |
| --- | --- | --- |
| `k1`, `k2`, `k3` | Radial distortion coefficients | Required, finite numbers |
| `p1`, `p2` | Tangential distortion coefficients | Required, finite numbers |
| `fx`, `fy` | Focal lengths | Required, finite and positive |
| `cx`, `cy` | Principal point coordinates | Required, finite numbers |
| `R` | Extrinsic rotation matrix | Optional 3&times;3 finite matrix |
| `t` | Extrinsic translation | Optional three-value/3&times;1 finite vector |
| `timestamp` | Calibration revision/time marker | Optional unsigned 64-bit integer |

The exported `projection_matrix` contains 16 values in row-major order. The C++
bridge deliberately requests a transpose during upload because OpenGL consumes
matrix values in column-major order by default.

## Rendering notes

- Texture coordinates are converted from `[0, 1]` UV space to centered `[-1, 1]`
  lens space before correction, then converted back before sampling.
- The inverse radial term is clamped away from zero to avoid singular behavior
  near the lens edge.
- Use clamp-to-border texture sampling when pixels outside the corrected source
  image should appear black.
- Static calibration uniforms are rebound only when sensors report a calibration
  change; eye offsets are uploaded on every frame.

## Repository structure

```text
.
├── .github/workflows/ci.yml       # Native and Python continuous integration
├── include/
│   └── optical_compensation.hpp   # Public C++ types and integration API
├── shaders/
│   ├── corrective.frag            # Fragment-stage texture correction
│   └── corrective.vert            # Vertex-stage geometry correction
├── src/
│   ├── optical_compensation.cpp   # JSON loading, uniforms, and runtime loop
│   └── perceptual_render_bridge.cpp # Embedded GLSL shader sources
├── tests/
│   ├── test_bridge.cpp            # Native bridge and validation tests
│   └── test_distortion_model.py   # Python calibration tests
├── CMakeLists.txt                 # C++17 library and test configuration
├── distortion_model.py            # Calibration, export, and sensor updates
├── LICENSE                        # MIT license
├── CITATION.cff                   # For Citation to Repository
├── PAPER.md                       # Architecture and mathematical background
└── README.md                      # Project overview and usage guide
```

## Scope and limitations

- The native JSON reader is intentionally focused on files emitted by
  `distortion_model.py`; it is not a general-purpose JSON parser.
- The library does not create a graphics context, own a window, load OpenGL
  functions, acquire device frames, or control frame presentation.
- Calibration quality, physical lens characterization, and hardware safety remain
  the integrator's responsibility.

## Project policies

Contributions are welcome; see [CONTRIBUTING.md](CONTRIBUTING.md) for the local
workflow. Participation is governed by the [Code of Conduct](CODE_OF_CONDUCT.md),
and vulnerabilities should follow the private process in [SECURITY.md](SECURITY.md).
Release history is maintained in [CHANGELOG.md](CHANGELOG.md).

## License

Distributed under the [MIT License](LICENSE).
