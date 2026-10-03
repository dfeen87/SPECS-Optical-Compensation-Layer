# Changelog

All notable changes to this project are documented in this file. The format is
based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases
follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-10-03

### Changed

- Established the BEDROCK engineering baseline with strict cross-language
  numeric, timestamp, JSON-structure, and runtime-state validation.
- Made calibration refresh validation precede frame capture and reject invalid
  state before rendering.
- Replaced Release-disabled native assertions with always-active test checks and
  expanded CI across Debug/Release and Python 3.10/current.

### Fixed

- Prevented malformed, nested, duplicate, or trailing JSON from being interpreted
  as trusted calibration data.
- Prevented timestamp truncation, projection overflow, malformed direct exports,
  and non-finite calibration or eye-offset values from crossing component boundaries.
- Added regression coverage for atomic rejection and shader cleanup failures.

See the [2.0.0 BEDROCK release notes](docs/RELEASE_2.0.0.md) for the invariant,
compatibility, CI, architecture, and external-validation details.

## [1.0.0] - 2026-09-27

### Added

- Dependency-free Python calibration validation and atomic JSON export.
- C++17 calibration loading, shader lifecycle helpers, and callback-driven
  rendering pipeline.
- Standalone and embedded GLSL 3.30 optical-correction shaders.
- Native and Python test suites, CMake package installation, and CI coverage.

[1.0.0]: https://github.com/dfeen87/SPECS-Optical-Compensation-Layer/releases/tag/v1.0.0

[2.0.0]: https://github.com/dfeen87/SPECS-Optical-Compensation-Layer/releases/tag/v2.0.0
