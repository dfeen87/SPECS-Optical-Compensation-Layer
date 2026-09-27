# SPECS Optical Compensation Layer

- **Version:** 1.0.0
- **Author:** Don M. Feeney Jr.
- **ORCID:** [0009-0003-1350-4160](https://orcid.org/0009-0003-1350-4160)
- **DOI:** [10.5281/zenodo.22102285](https://doi.org/10.5281/zenodo.22102285)

## Abstract

The SPECS Optical Compensation Layer is a dependency-light reference pipeline
for converting camera and lens calibration into GPU correction parameters. A
standard-library-only Python module validates Brown–Conrady coefficients and
camera transforms, then writes a JSON interchange file. A C++17 library reads
that file, manages shader parameters, and exposes a callback-driven render loop.
GLSL 3.30 shaders perform radial and tangential inverse mapping.

The repository deliberately stops at the integration boundary. It does not
provide a device SDK adapter, graphics-context creation, OpenGL function
loading, or calibration measurement. Applications supply those facilities
through narrow interfaces.

## 1. Architecture

The implementation has three layers:

1. **Calibration (`distortion_model.py`)** validates intrinsic and extrinsic
   parameters, computes a row-major projection matrix, and atomically exports
   JSON.
2. **Runtime (`src/optical_compensation.cpp`)** loads calibration, binds static
   and dynamic uniforms, and sequences host callbacks.
3. **Correction (`shaders/`)** maps geometry and sampled image coordinates with
   Brown–Conrady coefficients. Equivalent shader sources are embedded in
   `src/perceptual_render_bridge.cpp` for applications that do not load shader
   files at runtime.

```text
calibration input
       |
       v
Python validation and K[R|t]
       |
       v
calibration.json
       |
       v
C++ host bridge ---- sensor offsets / source textures
       |
       v
GLSL correction ---- corrected output for host presentation
```

The JSON file is an offline or infrequently refreshed interchange boundary;
Python is not invoked in the frame loop.

## 2. Calibration model

The required scalar inputs are the radial coefficients `k1`, `k2`, and `k3`,
the tangential coefficients `p1` and `p2`, and camera intrinsics `fx`, `fy`,
`cx`, and `cy`. All values must be finite, and both focal lengths must be
positive.

Rotation `R` defaults to the 3×3 identity matrix. Translation `t` defaults to a
three-element zero column and may be supplied as a flat vector or a 3×1 matrix.
The projection matrix is

$$P = K[R \mid t], \qquad
K = \begin{bmatrix}
f_x & 0 & c_x \\
0 & f_y & c_y \\
0 & 0 & 1
\end{bmatrix}.$$

The resulting three rows are extended with `[0, 0, 0, 1]` to produce the 4×4
matrix used by the shader interface. Python serializes it in row-major order;
the C++ bridge requests transposition when uploading it to OpenGL.

A `timestamp` may identify a calibration revision. Its accepted range is the
full unsigned 64-bit integer range. It defaults to zero and is metadata rather
than wall-clock time.

## 3. Distortion correction

For centered lens coordinates $(x,y)$, let

$$r^2=x^2+y^2, \quad r^4=(r^2)^2, \quad r^6=r^4r^2.$$

The radial factor is

$$d=1+k_1r^2+k_2r^4+k_3r^6.$$

Tangential terms are

$$x_t=2p_1xy+p_2(r^2+2x^2),$$

$$y_t=p_1(r^2+2y^2)+2p_2xy.$$

The reference shaders use the direct inverse approximation

$$x'=\frac{x-x_t}{\max(d,10^{-6})}, \qquad
y'=\frac{y-y_t}{\max(d,10^{-6})}.$$

The lower bound prevents division by zero. This approximation is intentionally
small and deterministic; it is not an iterative inversion. Integrators should
validate its accuracy against their lens model and calibration domain.

The vertex shader applies correction and the supplied projection to geometry.
The fragment shader converts UV coordinates from `[0,1]` to centered `[-1,1]`
space, applies correction, and converts back for sampling. Eye offsets are
included dynamically. Applications choose geometry and texture state
appropriate to whether they need geometry correction, image resampling, or the
combined reference behavior.

## 4. Native integration contract

`specs::GraphicsApi` abstracts only the graphics operations used by the
reference pipeline: shader compilation and linking, shader cleanup, program and
uniform binding, texture binding, and drawing. Compile and link failures are
reported by the host implementation, normally as exceptions. Intermediate
shader objects are deleted on success and failure; the caller owns the linked
program.

`specs::PipelineCallbacks` supplies the runtime-specific operations:

- determine whether rendering should continue;
- poll sensor state;
- acquire the next texture;
- refresh calibration when requested;
- present the completed frame; and
- synchronize frame timing.

Before entering the loop, the bridge validates required callbacks and the draw
vertex count, then binds static calibration uniforms. Each frame polls sensors
before capture, refreshes static calibration only when
`calibrationChanged` is true, uploads the eye offset, binds the captured
texture, draws, presents, and synchronizes. The host controls threading,
context affinity, error recovery, and timing policy.

## 5. Interchange format

A generated calibration document has this form:

```json
{
  "k1": -0.12,
  "k2": 0.03,
  "k3": 0.0,
  "p1": 0.0,
  "p2": 0.0,
  "projection_matrix": [
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  ],
  "timestamp": 1
}
```

The native reader is intentionally limited to the shape emitted by the Python
module. It is not a general JSON API. Producers other than
`export_calibration` should preserve these field names and numeric constraints.

Export uses a unique temporary file in the destination directory, flushes it,
and atomically replaces the destination. Readers therefore do not observe a
partially written document, including when multiple writers use the same path.

## 6. Scope, safety, and performance

The library introduces no mandatory third-party runtime dependencies. This
reduces integration surface but does not establish a latency guarantee. Actual
latency depends on device capture, context and driver behavior, texture
transfer, presentation, and the host's synchronization strategy. Integrators
must measure the complete deployed system.

This project is a reference implementation, not a medical device or a complete
wearable application. It does not characterize a lens, assess visual safety,
or validate calibration quality. Production users are responsible for
hardware-specific verification, out-of-range sampling behavior, numerical
bounds, per-eye configuration, fail-safe behavior, and all applicable safety
and regulatory requirements.

## 7. Reproducibility

The C++ implementation requires CMake 3.16 or newer and a C++17 compiler. The
calibration module requires Python 3 and uses only the standard library. Build
and execute both test suites with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python -m unittest discover -s tests -p 'test_*.py'
```

Version 1.0.0 is declared by the CMake project, public C++ version constants,
the citation metadata, and the changelog.
