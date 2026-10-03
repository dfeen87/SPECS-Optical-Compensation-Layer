# SPECS Optical Compensation Layer 2.0.0 — BEDROCK baseline

## Why this is a major release

Version 2.0.0 is a bottom-up hardening release. The three-layer calibration,
native bridge, and GLSL architecture remains intact; the guarantees below its
existing interfaces are now explicit and continuously tested. The project
claims Semantic Versioning, and this major increment records deliberate
behavioral incompatibilities: inputs that were coercible, structurally
malformed, non-finite, or not representable by the native layer are now
rejected rather than passed onward.

## Explicit invariants and corrected failure behavior

The release establishes these cross-layer contracts:

- Every calibration coefficient and projection element that reaches C++ or a
  graphics callback is finite and representable as a C++ `float`.
- A timestamp is an actual unsigned 64-bit integer. Boolean, floating-point,
  numeric-string, fractional, negative, and overflowing forms are invalid.
- The JSON interchange document is structurally valid. Required values must be
  top-level, required fields may occur only once, the projection has exactly 16
  numeric elements, and trailing content is rejected.
- Export validates a complete candidate document before creating directories or
  temporary files. A rejected export therefore leaves an existing destination
  unchanged and creates no partial artifact.
- Runtime calibration and sensor evidence fail closed. Invalid initial values
  cause no graphics calls; invalid sensor offsets consume no frame; and an
  invalid calibration refresh is neither committed nor drawn.
- Shader objects created before a compile or link failure remain owned by the
  bridge until it deletes them. The linked program remains caller-owned.
- Frame ordering remains sensor poll, optional validated calibration refresh,
  capture, dynamic upload, draw, presentation, and synchronization.

Previously, the focused native reader searched text with regular expressions.
That could accept a required-looking field nested inside unrelated data, ignore
trailing garbage, accept duplicate fields ambiguously, or extract numbers from
a malformed matrix. The dependency-free reader now parses the document
structure before trusting values. Python previously coerced numeric strings and
booleans and truncated a sensor timestamp before validation; those forms are
now rejected consistently. Projection overflow and malformed direct export
were also not checked at the producer boundary.

## Regression and CI guarantees

Native regression tests now cover structural JSON rejection, duplicate and
nested impostor fields, trailing content, matrix corruption, numeric bounds,
shader cleanup on both failure paths, callback ordering, and fail-closed runtime
validation. The native test executable uses exception-based checks rather than
C `assert`, so Release builds cannot compile its proofs away. Python regression
tests cover type and float-range boundaries, projection overflow, malformed
matrix shapes, timestamp forms, sensor-update non-mutation, complete export
validation, and preservation of an existing destination on rejection.

CI runs the native suite in both Debug and Release and runs the Python suite on
Python 3.10 (the declared minimum) and the current hosted Python 3 release. This
makes build-mode and minimum-runtime assumptions permanent checks rather than
local conventions.

## Compatibility and intentionally preserved architecture

No integration interface was removed: `compute_calibration`,
`export_calibration`, `update_from_sensor`, `GraphicsApi`, `PipelineCallbacks`,
and the shader interfaces retain their roles. Unknown top-level JSON members
remain tolerated for forward-compatible metadata, and an omitted timestamp
still defaults to zero in the native reader. The C++17 and standard-library-only
implementation, host-owned graphics context, callback-driven render loop,
row-major interchange matrix, and GLSL 3.30 correction model are preserved.

Version 2.0.0 rejects behavior that 1.x could accept accidentally: coercible
non-number Python values, floating-point timestamps (including integral ones),
values outside native float range, computed projection overflow, incomplete
shader-ready exports, non-finite runtime state, duplicate JSON members,
required-looking nested members, malformed arrays, and trailing JSON content.
These are contract corrections at calibration and safety boundaries, not new
features.

## Remaining validation outside this repository

Software tests do not certify optical accuracy, human visual safety, latency,
or hardware suitability. Integrators must still perform calibration-quality
analysis, simulation validation, device/driver and OpenGL integration testing,
hardware-in-the-loop testing, per-eye and out-of-range sampling validation,
performance and failure-mode measurement, and any applicable medical, product,
or regulatory review. The direct inverse Brown–Conrady approximation and the
`1e-6` radial lower bound must be validated against the deployed lens domain.
The project has no related trust framework or external device SDK in-tree, so
compatibility with any such repository is not asserted by this release.
