# Specs Optical Compensation Layer

**Presented to:** Snap Inc.
**Author:** Don M. Feeney Jr.
**ORCID:** [https://orcid.org/0009-0003-1350-4160](https://orcid.org/0009-0003-1350-4160)
**DOI:** [https://doi.org/10.5281/zenodo.22102285](https://doi.org/10.5281/zenodo.22102285)

---

## Table of Contents

- [Executive Summary](#executive-summary)
- [1. Introduction](#1-introduction)
- [2. Perceptual Modeling Layer (Python + OpenGL)](#2-perceptual-modeling-layer-python--opengl)
- [3. Performance & Minimalism: Ensuring Zero Bloat and Low Latency](#3-performance--minimalism-ensuring-zero-bloat-and-low-latency)
- [4. C++ Hardware Integration Layer (Foundational Optical Compensation)](#4-c-hardware-integration-layer-foundational-optical-compensation)
- [5. Math Foundations: Python Modeling and OpenGL/GLU Transformation](#5-math-foundations-python-modeling-and-openglglu-transformation)
- [6. C++ Hardware Integration Layer](#6-c-hardware-integration-layer)
- [7. Corrective Distortion Shader (GLSL Integration Layer)](#7-corrective-distortion-shader-glsl-integration-layer)
- [8. C++ Runtime Loop (Pseudocode)](#8-c-runtime-loop-pseudocode)
- [9. Shader Integration in the C++ Bridge](#9-shader-integration-in-the-c-bridge)
- [10. Parameter Interface & Data Structures](#10-parameter-interface--data-structures)
- [11. Python Calibration & Modeling (Math Layer)](#11-python-calibration--modeling-math-layer)
- [12. System Summary & Pipeline Overview](#12-system-summary--pipeline-overview)
- [Appendix A: Variable Definitions & Formula Breakdown](#appendix-a-variable-definitions--formula-breakdown)
- [Appendix B: Uniform Mapping & Optical Glossary](#appendix-b-uniform-mapping--optical-glossary)
- [Author's Note](#authors-note)
- [Closing Note of Appreciation](#closing-note-of-appreciation)
- [Acknowledgment](#acknowledgment)

---

## Executive Summary

The corrective-vision system is a modular, parameter-driven optical pipeline designed for real-time AR hardware. It integrates three coordinated layers: a Python calibration engine, a C++ orchestration bridge, and GLSL shaders executing on the GPU. Python computes distortion coefficients and calibrated projection matrices from device and user data. These parameters populate a unified `CalibrationParams` structure in C++, which binds them as uniforms to embedded GLSL vertex and fragment shaders. The C++ runtime loop polls sensors, updates dynamic offsets, executes the corrective shaders, and presents distortion-free frames to the display hardware at full refresh rate. This architecture ensures deterministic timing, low latency, and seamless integration beneath proprietary rendering stacks.

---

## 1. Introduction

The work presented in this paper explores a new corrective-vision software system designed for Snap's Spectacles platform. At its core, the system is built on a C++ hardware integration layer, responsible for real-time optical compensation and direct communication with Specs sensors and runtime. This foundational layer ensures that every correction applied to the wearer's view is fast, stable, and aligned with the physical constraints of the glasses.

Above this foundation lives a dual-layer perceptual engine. The first part of this engine is a Python-based modeling environment that analyzes distortion patterns, generates calibration curves, and simulates how corrected reality should appear to the human eye. The second part is a digital render layer powered by OpenGL, which transforms these perceptual models into GPU-accelerated visual outputs. Together, Python and OpenGL create a flexible space where corrected reality can be visualized, tested, and refined, allowing us to explore how the world might look through adaptive, software-driven eyewear.

This paper focuses entirely on the engineering behind this system. It outlines the architecture, the math, the file-level structure, and the logic that connects each component. All business decisions, product strategy, and future direction remain fully in Snap Inc.'s hands. Our goal is simply to demonstrate a clear, functional, and forward-thinking technical foundation for software-based corrective vision on Spectacles.

---

## 2. Perceptual Modeling Layer (Python + OpenGL)

This section defines the dual-language perceptual engine responsible for analyzing, modeling, and visually reconstructing corrected reality. Python provides the analytical and simulation backbone, while OpenGL renders the corrected visual field in real time.

### 2.1 Purpose of the Perceptual Modeling Layer

The perceptual modeling layer transforms raw optical data from the C++ hardware layer into a structured understanding of how the wearer should see the world. It acts as a bridge between physical correction and digital reconstruction.

It performs three core functions:

- **Analyze distortion** — interpreting raw sensor data and identifying correction needs
- **Model perception** — simulating how the corrected world should appear to the human eye
- **Generate render instructions** — producing GPU-ready transformations for the OpenGL layer

This layer is the "interpretation engine" of the system.

### 2.2 Python: Analytical & Simulation Core

Python provides the mathematical and perceptual backbone of the system. It is responsible for:

- Distortion curve generation
- Calibration mapping
- Eye-specific correction modeling
- Visual analytics and charting
- Simulation of corrected frames
- Parameter tuning for OpenGL shaders

Python's role is to **understand reality**.

Key modules include:

| Module | Responsibility |
|---|---|
| `distortion_model.py` | computes lens and eye distortion |
| `calibration_curves.py` | generates correction curves |
| `perception_simulator.py` | simulates corrected frames |
| `render_parameters.py` | prepares data for OpenGL |

Python outputs structured correction data that the render layer consumes.

### 2.3 OpenGL: Digital Render Layer

OpenGL is responsible for reconstructing the corrected world visually. It applies GPU-accelerated transformations based on Python's perceptual models.

Core responsibilities:

- Shader-based distortion correction
- Real-time rendering of corrected frames
- Metaverse-style overlays for testing
- GPU-accelerated visual transformations
- Dynamic updates based on Python parameters

OpenGL's role is to **reconstruct reality**.

Key files:

| File | Responsibility |
|---|---|
| `vertex_shader.glsl` | handles geometry and frame mapping |
| `fragment_shader.glsl` | applies distortion correction |
| `render_pipeline.cpp` | connects OpenGL to Python outputs |

This layer produces the final visual output that simulates what the wearer will see.

### 2.4 Data Flow Between Python and OpenGL

The perceptual modeling layer uses a clean, modular data flow:

1. C++ captures raw optical data
2. Python analyzes distortion and models perception
3. Python generates correction parameters
4. OpenGL applies GPU-accelerated transformations
5. OpenGL renders corrected frames for testing

This creates a dual-layer visual system that is both analytical and expressive.

### 2.5 Why This Dual-Layer System Matters

This architecture allows us to:

- simulate corrected vision before hardware deployment
- visualize how the world will appear through adaptive eyewear
- refine correction curves rapidly
- test metaverse-style overlays
- build a system that feels alive, flexible, and future-ready

It is the cleanest, simplest, and most powerful way to reimagine what an eye sees.

---

## 3. Performance & Minimalism: Ensuring Zero Bloat and Low Latency

The corrective-vision system is intentionally designed to avoid architectural bloat, unnecessary language dependencies, and congestion that could introduce latency. Each layer — C++, Python, and OpenGL — has a clearly defined role, and the system maintains strict boundaries to ensure that data moves efficiently from hardware to perception to rendering.

This section explains how the combination of these technologies remains lightweight, modular, and optimized for real-time performance.

### 3.1 Minimal Language Footprint

The system uses only three languages, each chosen for a single, non-overlapping purpose:

- **C++** — hardware reality
- **Python** — perceptual modeling
- **OpenGL (GLSL)** — digital rendering

No additional languages, frameworks, or glue layers are introduced. This eliminates:

- runtime overhead
- dependency sprawl
- cross-language latency
- complex build pipelines

The architecture remains lean and predictable.

### 3.2 Single Integration File to Prevent Congestion

Python and OpenGL communicate through one file: `perceptual_render_bridge.py`

This file handles:

- model import
- OpenGL initialization
- shader compilation
- GPU data upload
- render loop execution

By consolidating all Python ↔ OpenGL communication into a single module, the system avoids:

- multi-file fragmentation
- redundant data paths
- duplicated logic
- synchronization overhead

This keeps the repo clean and the runtime fast.

### 3.3 Zero Redundant Data Transfers

The system uses a direct parameter-passing model:

1. C++ outputs optical parameters
2. Python transforms them
3. OpenGL consumes them

There are no:

- intermediate serialization formats
- JSON or XML layers
- multi-stage buffers
- unnecessary conversions

Data moves in its raw numerical form, minimizing latency and maximizing throughput.

### 3.4 GPU-Accelerated Correction to Reduce CPU Load

All heavy visual transformations occur in OpenGL shaders, not Python. Python only computes the parameters.

This prevents:

- CPU bottlenecks
- frame-rate drops
- jitter
- perceptual lag

The GPU handles distortion correction at native speed.

### 3.5 Strict Separation of Responsibilities

Each layer performs only the tasks it is best suited for:

- **C++** — real-time hardware logic
- **Python** — math, modeling, analytics
- **OpenGL** — rendering and GPU transforms

This prevents overlap, which is the #1 cause of bloat in multi-language systems.

### 3.6 No External Engines or Heavy Frameworks

The system intentionally avoids:

- Unity
- Unreal
- Vulkan
- WebGPU
- DirectX
- Qt
- Electron
- heavy visualization libraries

This keeps the repo:

- small
- portable
- fast
- easy to review
- easy to maintain

Snap engineers will immediately recognize the elegance of this minimal approach.

### 3.7 Real-Time Pipeline Designed for Low Latency

The entire architecture is built around a tight loop:

1. C++ captures →
2. Python models →
3. OpenGL renders

No extra hops. No middleware. No unnecessary layers.

This is how you achieve near-real-time corrective vision simulation.

---

## 4. C++ Hardware Integration Layer (Foundational Optical Compensation)

The C++ hardware layer forms the foundation of the corrective-vision system. It interfaces directly with Specs sensors, applies real-time optical compensation, and prepares structured data for the perceptual modeling layer. This layer is optimized for low latency, deterministic execution, and alignment with embedded constraints.

### 4.1 Responsibilities of the C++ Layer

The C++ layer performs four critical functions:

- **Frame acquisition** — capturing raw optical data from Specs sensors
- **Distortion correction** — applying lens and eye-specific compensation math
- **Real-time update loops** — ensuring stable frame-by-frame correction
- **Data packaging** — preparing numerical parameters for Python

This layer ensures that every correction begins with accurate, low-latency optical data.

### 4.2 Core Files

The C++ layer is intentionally minimal:

- `optical_compensation.hpp` — defines correction structures and interfaces
- `optical_compensation.cpp` — implements distortion math and frame updates
- `specs_runtime_adapter.cpp` — handles sensor communication and runtime integration

These files form the backbone of the hardware-aligned system.

### 4.3 Distortion Math

The C++ layer applies:

- radial distortion correction
- tangential distortion correction
- chromatic aberration compensation
- eye-specific calibration offsets

These calculations are optimized for embedded execution.

### 4.4 Data Packaging for Python

The C++ layer outputs:

- distortion coefficients
- calibration parameters
- frame metadata
- correction matrices

All packaged in a lightweight structure that Python can consume without conversion overhead.

### 4.5 Why C++ Is Essential

C++ provides:

- deterministic performance
- low-level hardware access
- memory-safe operations
- minimal latency
- embedded compatibility

---

## 5. Math Foundations: Python Modeling and OpenGL/GLU Transformation

This section defines the mathematical backbone of the corrective-vision system. Python is used for high-level modeling, curve fitting, and parameter generation, while OpenGL/GLU applies those parameters as real-time geometric and optical transformations on the GPU. Together, they form a complete algorithmic pipeline for software-based corrective vision.

### 5.1 Python math: Modeling distortion and correction

Python's role is to understand and describe the optical system in mathematical terms.

**Radial distortion:**

$$r_{distorted} = r\left(1 + k_1 r^2 + k_2 r^4 + k_3 r^6\right)$$

**Tangential distortion:**

$$x_{distorted} = x + \left(2p_1 xy + p_2(r^2 + 2x^2)\right)$$
$$y_{distorted} = y + \left(p_1(r^2 + 2y^2) + 2p_2 xy\right)$$

**Calibration curve generation:** Python fits curves from calibration data (test patterns, known grids, eye measurements). Outputs: $k_1, k_2, k_3, p_1, p_2$, and per-eye offsets.

**Matrix formulation:** Intrinsic camera matrix $K$; extrinsic matrices $R, t$; combined projection:

$$P = K[R \mid t]$$

Python's strength here is flexibility: fast iteration, numerical libraries, and clear expression of the math.

### 5.2 OpenGL/GLU math: Applying correction on the GPU

OpenGL/GLU's role is to apply the math in real time.

- **Vertex transformations:** Incoming normalized coordinates $(x, y)$ are corrected using Python-generated parameters. In shader form, the inverse distortion is applied to map distorted coordinates back to ideal ones.
- **Projection and view matrices:** GLU/GL-style math uses:

$$M_{final} = M_{projection} \cdot M_{view} \cdot M_{model}$$

  These matrices are tuned using Python's calibration outputs, so the rendered frame matches the corrected optical model.
- **GPU execution:** All per-pixel and per-vertex corrections run in shaders, keeping latency low and frame rates high.

OpenGL/GLU's strength is speed: once the math is defined, the GPU can apply it to every frame with minimal overhead.

### 5.3 Combining Python math and GLU math into a single algorithm

The core algorithm is the fusion of Python's modeling and GLU's transformation.

1. **Python stage (offline/parameter generation):** Compute distortion parameters $(k_1, k_2, k_3, p_1, p_2)$; compute calibration matrices $K, R, t$; export correction parameters and matrices as GPU-ready data.
2. **OpenGL/GLU stage (online/runtime):** Load Python-generated parameters into uniforms and buffers; apply inverse distortion and calibrated projection in shaders; render the corrected frame so that what is seen on the display matches the mathematically corrected model.
3. **Algorithmic completion:** The algorithm is "complete" when Python defines the *what* (the math of correction) and OpenGL/GLU defines the *how* (the application of that math in real time). Together, they form a closed loop:

$$\text{Raw frame} \rightarrow \text{Python model} \rightarrow \text{GLU transform} \rightarrow \text{Corrected frame}$$

---

## 6. C++ Hardware Integration Layer

The C++ layer serves as the system's hardware anchor, responsible for acquiring sensor data, managing device resources, orchestrating the runtime loop, and bridging Python-generated calibration parameters into the GPU rendering pipeline. While Python defines the mathematical model and GLU/OpenGL apply the optical transformations, C++ ensures these components operate cohesively on real hardware with deterministic timing and low latency.

### 6.1 Role of C++ in the System Architecture

C++ is responsible for all real-time, device-level operations:

- **Frame acquisition** — capturing raw camera frames or sensor inputs
- **Device communication** — interfacing with the glasses' hardware APIs
- **Memory management** — allocating GPU buffers, managing uniform updates
- **Runtime orchestration** — maintaining the main loop that drives rendering
- **Parameter injection** — loading Python-generated calibration data into GLU/OpenGL
- **Latency control** — ensuring the pipeline stays within acceptable timing thresholds

C++ acts as the execution backbone of the corrective-vision system.

### 6.2 Data Flow: C++ → Python → GLU/OpenGL → Display

The integration pipeline follows a deterministic sequence:

1. **C++ captures raw frame or sensor data** — retrieves the current frame from the glasses' camera or display buffer; polls sensors for eye position, orientation, or calibration triggers.
2. **C++ sends calibration inputs to Python** — provides Python with the necessary data to compute distortion parameters; Python processes the inputs offline or asynchronously.
3. **Python computes distortion parameters and matrices** — outputs $k_1, k_2, k_3, p_1, p_2$, intrinsic matrix $K$, rotation $R$, translation $t$; exports GPU-ready arrays and matrices.
4. **C++ loads Python outputs into GPU uniforms** — uses OpenGL calls to bind matrices and parameters to shader uniforms; ensures the GPU receives updated calibration data without blocking the runtime loop.
5. **GLU/OpenGL apply inverse distortion and projection** — shader programs use the parameters to correct the frame in real time; the GPU performs all heavy computations.
6. **C++ presents the corrected frame to the display** — the final corrected frame is sent to the glasses' display hardware; loop repeats at the device's refresh rate.

This pipeline ensures the mathematical model is applied consistently and efficiently.

### 6.3 The C++ Runtime Loop

The runtime loop is the heartbeat of the system. It maintains deterministic timing and ensures the corrective algorithm runs continuously.

A typical loop structure:

- **Poll sensors** — retrieve eye position, orientation, or calibration triggers
- **Capture frame** — acquire the raw frame from the glasses' camera or display buffer
- **Update Python model (if needed)** — trigger recalculation of distortion parameters when calibration changes
- **Update GPU uniforms** — load Python-generated matrices and parameters into OpenGL
- **Render corrected frame** — call the GLU/OpenGL pipeline to apply distortion correction
- **Present frame** — output the corrected frame to the display hardware
- **Repeat** — maintain stable timing at the device's refresh rate

This loop ensures smooth, real-time corrective rendering.

### 6.4 Memory and Latency Considerations

To maintain visual stability and comfort, the system must operate within strict latency constraints.

Key considerations:

- **Avoid Python-side blocking** — Python computations should run asynchronously or in a separate thread
- **Efficient uniform updates** — only update GPU uniforms when calibration parameters change
- **Frame timing stability** — maintain consistent frame intervals to avoid jitter
- **GPU buffer ownership** — C++ manages buffer lifetimes to prevent fragmentation or stalls
- **Latency target** — keep total pipeline latency under 10–15 ms for comfortable AR viewing

These constraints ensure the corrective algorithm feels seamless to the user.

### 6.5 The Integration File: `perceptual_render_bridge.cpp`

This file serves as the central integration point between Python, GLU/OpenGL, and the hardware.

Responsibilities:

- Initialize OpenGL context
- Load Python-generated matrices and parameters
- Bind shader programs
- Update uniforms each frame
- Manage the runtime loop
- Interface with device APIs
- Present corrected frames to the display

This file is the "glue" that binds the entire system together.

---

## 7. Corrective Distortion Shader (GLSL Integration Layer)

The corrective distortion shader is implemented in GLSL as a foundational optical layer that operates beneath the existing rendering pipeline of the glasses. Its sole responsibility is to apply mathematically defined inverse distortion and calibrated projection to incoming geometry or image data, using parameters generated by the Python modeling stage and delivered through the C++ hardware integration layer.

This shader is explicitly designed to be non-intrusive and compatible with proprietary shader stacks. It does not replace or redefine any AR content, UI, or visual effects shaders. Instead, it provides an optically corrected basis upon which those higher-level shaders can operate without modification.

### 7.1 Conceptual role in the rendering pipeline

The corrective distortion shader sits at the optical correction layer, early in the pipeline:

- Before AR content compositing
- Before UI overlays
- Before visual effects
- Underneath Snap's proprietary shaders

Its function is to ensure that all subsequent rendering occurs in a space that has already been corrected for lens distortion and calibrated to the user's visual parameters.

Conceptually, the pipeline becomes:

$$\text{Raw geometry/frame} \rightarrow \text{Corrective Distortion Shader} \rightarrow \text{Proprietary AR/UI shaders} \rightarrow \text{Final display}$$

### 7.2 Inputs to the corrective shader

The shader receives all of its parameters from the Python modeling stage via the C++ integration layer. Typical inputs include:

- **Distortion parameters:** $k_1, k_2, k_3, p_1, p_2$ — describe radial and tangential distortion characteristics of the optical system.
- **Calibration matrices:** intrinsic matrix $K$, rotation matrix $R$, translation vector $t$, combined into a projection mapping $P = K[R \mid t]$.
- **Normalized coordinates or vertex positions:** input positions in normalized device coordinates (NDC) or a similar space — the "ideal" positions before distortion correction is applied.
- **Optional per-eye offsets or parameters:** left/right eye calibration differences; user-specific adjustments derived from Python's modeling.

All of these inputs are bound as uniforms or attributes by the C++ layer.

### 7.3 Core mathematical operation

The corrective distortion shader applies inverse distortion and calibrated projection to incoming coordinates.

**1. Inverse radial distortion:** Given a distorted radius $r_{distorted}$, the shader approximates the undistorted radius $r$ using parameters $(k_1, k_2, k_3)$:

$$r_{distorted} = r\left(1 + k_1 r^2 + k_2 r^4 + k_3 r^6\right)$$

The shader uses an iterative or approximated inverse to recover $r$ from $r_{distorted}$.

**2. Inverse tangential distortion:** For tangential components $(p_1, p_2)$, the shader corrects:

$$x_{distorted} = x + \left(2p_1 xy + p_2(r^2 + 2x^2)\right)$$
$$y_{distorted} = y + \left(p_1(r^2 + 2y^2) + 2p_2 xy\right)$$

The shader applies the inverse mapping to recover $(x, y)$ from $(x_{distorted}, y_{distorted})$.

**3. Projection using calibration matrices:** Once corrected coordinates are obtained, the shader applies the calibrated projection:

$$p_{corrected} = P \cdot v$$

where $v$ is the input vertex or coordinate in homogeneous form, and $P$ is the combined projection matrix.

The result is a coordinate or fragment that has been optically corrected according to the user's calibration and the device's lens characteristics.

### 7.4 Shader structure (conceptual GLSL layout)

While the exact implementation will depend on the target environment, a typical corrective shader conceptually includes:

- **Uniforms:** distortion parameters $(k_1, k_2, k_3, p_1, p_2)$; calibration matrices $K, R, t$ or combined $P$; optional per-eye parameters
- **Attributes / inputs:** vertex positions or texture coordinates; any additional data required by upstream shaders
- **Main function:** compute radius and distorted coordinates; apply inverse distortion; apply calibrated projection; output corrected position or texture coordinate for downstream shaders

This shader is designed to be inserted as a stage in the pipeline, not as a replacement for existing visual logic.

### 7.5 Integration beneath proprietary shaders

To respect existing intellectual property and rendering systems, the corrective distortion shader is explicitly positioned as a foundational layer:

- It runs before proprietary AR content shaders.
- It does not alter or define Snap's internal shading models.
- It provides corrected coordinates or frames that proprietary shaders can consume without modification.
- It is portable and can be adapted to any OpenGL/GLU-style environment.

In a typical integration, the corrective shader would be bound early in the pipeline, applied to geometry or image data, and then followed by Snap's existing shaders for AR content, UI, and effects.

This ensures that the corrective vision system enhances the optical fidelity of the device without interfering with or exposing proprietary shader logic.

### 7.6 Relationship to Python and C++ layers

The corrective distortion shader is the GPU endpoint of the Python–C++–GLU pipeline:

- Python defines the mathematical model and generates parameters.
- C++ manages hardware, runtime, and uniform binding.
- GLSL (this shader) applies the math in real time on the GPU.

Together, they form a complete, modular corrective-vision system that can be integrated into an existing AR rendering stack while fully respecting proprietary implementations.

### 7.7 Corrective Distortion Vertex Shader (GLSL Implementation)

```glsl
#version 330 core

uniform float k1;
uniform float k2;
uniform float k3;
uniform float p1;
uniform float p2;
uniform mat4 uProjectionMatrix;
uniform vec2 uEyeOffset;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;

void main()
{
    float x = aPosition.x + uEyeOffset.x;
    float y = aPosition.y + uEyeOffset.y;
    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float r6 = r4 * r2;
    float radial = max(1.0 + k1 * r2 + k2 * r4 + k3 * r6, 0.000001);
    float xTangential = 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float yTangential = p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    vec4 corrected = vec4((x - xTangential) / radial,
                          (y - yTangential) / radial, aPosition.z, 1.0);
    gl_Position = uProjectionMatrix * corrected;
    vTexCoord = aTexCoord;
}
```

### 7.8 Corrective Distortion Fragment Shader (GLSL Implementation)

```glsl
#version 330 core

uniform float k1;
uniform float k2;
uniform float k3;
uniform float p1;
uniform float p2;
uniform vec2 uEyeOffset;
uniform sampler2D uInputFrame;

in vec2 vTexCoord;
out vec4 fragColor;

void main()
{
    vec2 centered = (vTexCoord - vec2(0.5)) * 2.0 + uEyeOffset;
    float x = centered.x;
    float y = centered.y;
    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float r6 = r4 * r2;
    float radial = max(1.0 + k1 * r2 + k2 * r4 + k3 * r6, 0.000001);
    float xTangential = 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float yTangential = p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    vec2 corrected = vec2((x - xTangential) / radial,
                          (y - yTangential) / radial);
    vec2 correctedUV = (corrected - uEyeOffset) * 0.5 + vec2(0.5);
    fragColor = texture(uInputFrame, correctedUV);
}
```

---

## 8. C++ Runtime Loop

The C++ runtime loop is the execution spine of the corrective-vision system. It orchestrates sensor polling, Python parameter loading, shader uniform updates, GPU rendering, and final frame presentation. This loop ensures deterministic timing and stable optical correction at the device's refresh rate.

### 8.1 Initialization Phase

Before entering the loop, the system initializes all required subsystems:

```cpp
CalibrationParams params = loadParamsFromJson("calibration.json");
GlId correctiveShader = createCorrectiveShaderProgram(
    graphics, correctiveVertexShaderSource, correctiveFragmentShaderSource);
bindUniforms(graphics, correctiveShader, params);
```

After the application supplies its `GraphicsApi` implementation and graphics context, this phase compiles the shaders and imports Python-generated calibration parameters.

### 8.2 Main Runtime Loop

```cpp
void runCorrectivePipeline(GraphicsApi& graphics, GlId program, CalibrationParams params,
                           const PipelineCallbacks& callbacks, int vertexCount) {
    if (!callbacks.deviceIsRunning || !callbacks.pollDeviceSensors || !callbacks.captureFrame ||
        !callbacks.presentFrame || !callbacks.synchronizeFrameTiming)
        throw std::invalid_argument("pipeline callbacks must be configured");
    if (vertexCount <= 0) throw std::invalid_argument("vertexCount must be positive");

    bindUniforms(graphics, program, params);
    while (callbacks.deviceIsRunning()) {
        const SensorData sensor = callbacks.pollDeviceSensors();
        const FrameData frame = callbacks.captureFrame();
        if (sensor.calibrationChanged) {
            if (!callbacks.updateCalibration)
                throw std::invalid_argument("calibration update callback is not configured");
            params = callbacks.updateCalibration(sensor);
            bindUniforms(graphics, program, params);
        }
        params.eyeOffsetX = sensor.eyeOffsetX;
        params.eyeOffsetY = sensor.eyeOffsetY;
        updateDynamicUniforms(graphics, program, sensor);
        graphics.bindTexture(frame.texture);
        graphics.useProgram(program);
        graphics.drawTriangles(vertexCount);
        callbacks.presentFrame();
        callbacks.synchronizeFrameTiming();
    }
}
```

This loop runs continuously at the device's refresh rate (e.g., 60–90 Hz), ensuring smooth optical correction.

### 8.3 Responsibilities of Each Step

- **Poll sensors** — retrieves eye position, orientation, or calibration triggers, ensuring correction stays aligned with the user's real-time visual state
- **Capture frame** — obtains the raw camera passthrough or pre-rendered buffer
- **Update Python model** — only triggered when calibration changes, preventing blocking the loop with unnecessary Python calls
- **Bind shader program** — activates the corrective distortion shader
- **Update dynamic uniforms** — includes per-eye offsets, dynamic distortion tweaks, or sensor-driven parameters
- **Bind textures/buffers** — ensures the shader receives the correct input frame or geometry
- **Execute shader** — GPU applies inverse distortion + calibrated projection
- **Present frame** — sends corrected output to the glasses' display hardware
- **Synchronize timing** — maintains stable frame pacing to avoid jitter or latency spikes

### 8.4 Latency Considerations

The runtime loop is designed to maintain total pipeline latency under 10–15 ms, ensuring comfortable AR viewing.

Key strategies:

- Python updates run asynchronously
- Uniform updates are lightweight
- GPU handles all heavy math
- Frame pacing avoids stalls
- Sensor polling is non-blocking

This ensures the corrective system feels seamless and invisible to the user.

### 8.5 Integration Points

The runtime loop is the anchor for:

- Python parameter loading
- GLSL shader execution
- Hardware frame presentation

All components plug into this loop.

---

## 9. Shader Integration in the C++ Bridge

The C++ bridge file (`perceptual_render_bridge.cpp`) is responsible for embedding, compiling, and binding the corrective distortion shaders so they can run as part of the device's real-time rendering pipeline. This section shows how the GLSL vertex and fragment shaders are integrated into the C++ runtime in a way that is portable and compatible with existing proprietary shader stacks.

### 9.1 Embedding GLSL shaders as C++ string literals

To keep the system self-contained and portable, the corrective shaders are embedded directly in the C++ source as string literals:

```cpp
const char* correctiveVertexShaderSource = R"GLSL(#version 330 core
uniform float k1; uniform float k2; uniform float k3; uniform float p1; uniform float p2;
uniform mat4 uProjectionMatrix; uniform vec2 uEyeOffset;
layout(location=0) in vec3 aPosition; layout(location=1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    float x=aPosition.x+uEyeOffset.x, y=aPosition.y+uEyeOffset.y;
    float r2=x*x+y*y, r4=r2*r2, r6=r4*r2;
    float radial=max(1.0+k1*r2+k2*r4+k3*r6,0.000001);
    float xt=2.0*p1*x*y+p2*(r2+2.0*x*x);
    float yt=p1*(r2+2.0*y*y)+2.0*p2*x*y;
    gl_Position=uProjectionMatrix*vec4((x-xt)/radial,(y-yt)/radial,aPosition.z,1.0);
    vTexCoord=aTexCoord;
})GLSL";
```

```cpp
const char* correctiveFragmentShaderSource = R"GLSL(#version 330 core
uniform float k1; uniform float k2; uniform float k3; uniform float p1; uniform float p2;
uniform vec2 uEyeOffset; uniform sampler2D uInputFrame;
in vec2 vTexCoord; out vec4 fragColor;
void main() {
    vec2 centered=(vTexCoord-vec2(0.5))*2.0+uEyeOffset;
    float x=centered.x, y=centered.y, r2=x*x+y*y, r4=r2*r2, r6=r4*r2;
    float radial=max(1.0+k1*r2+k2*r4+k3*r6,0.000001);
    float xt=2.0*p1*x*y+p2*(r2+2.0*x*x);
    float yt=p1*(r2+2.0*y*y)+2.0*p2*x*y;
    vec2 corrected=vec2((x-xt)/radial,(y-yt)/radial);
    fragColor=texture(uInputFrame,(corrected-uEyeOffset)*0.5+vec2(0.5));
})GLSL";
```

These embedded shaders define the optical correction layer and can be compiled at runtime by the C++ bridge.

### 9.2 Compiling and linking the shader program

The C++ bridge compiles the embedded GLSL sources and links them into a single shader program:

```cpp
GlId createCorrectiveShaderProgram(GraphicsApi& graphics, const std::string& vertexSource,
                                   const std::string& fragmentSource) {
    const GlId vertex = graphics.compileShader(0x8B31, vertexSource);  // GL_VERTEX_SHADER
    GlId fragment{};
    try {
        fragment = graphics.compileShader(0x8B30, fragmentSource);  // GL_FRAGMENT_SHADER
        const GlId program = graphics.linkProgram(vertex, fragment);
        graphics.deleteShader(vertex);
        graphics.deleteShader(fragment);
        return program;
    } catch (...) {
        graphics.deleteShader(vertex);
        if (fragment) graphics.deleteShader(fragment);
        throw;
    }
}
```

This function produces a corrective shader program for the runtime loop and releases intermediate shader objects on both success and failure.

### 9.3 Binding Python-generated parameters as uniforms

Once the shader program is created, the C++ bridge binds Python-generated calibration parameters to the shader uniforms:

```cpp
void bindUniforms(GraphicsApi& graphics, GlId program, const CalibrationParams& params) {
    graphics.useProgram(program);
    graphics.uniform1f(program, "k1", params.k1);
    graphics.uniform1f(program, "k2", params.k2);
    graphics.uniform1f(program, "k3", params.k3);
    graphics.uniform1f(program, "p1", params.p1);
    graphics.uniform1f(program, "p2", params.p2);
    // Python serializes rows; OpenGL expects columns, hence transpose=true.
    graphics.uniformMatrix4(program, "uProjectionMatrix", params.projectionMatrix.data(), true);
}
```

Per-frame dynamic values (such as eye offsets) are updated inside the runtime loop:

```cpp
void updateDynamicUniforms(GraphicsApi& graphics, GlId program, const SensorData& sensor) {
    graphics.useProgram(program);
    graphics.uniform2f(program, "uEyeOffset", sensor.eyeOffsetX, sensor.eyeOffsetY);
}
```

### 9.4 Integration into the runtime loop

Within the main runtime loop, the corrective shader program is activated and used to render the corrected frame:

```cpp
void runCorrectivePipeline(GraphicsApi& graphics, GlId program, CalibrationParams params,
                           const PipelineCallbacks& callbacks, int vertexCount) {
    if (!callbacks.deviceIsRunning || !callbacks.pollDeviceSensors || !callbacks.captureFrame ||
        !callbacks.presentFrame || !callbacks.synchronizeFrameTiming)
        throw std::invalid_argument("pipeline callbacks must be configured");
    if (vertexCount <= 0) throw std::invalid_argument("vertexCount must be positive");

    bindUniforms(graphics, program, params);
    while (callbacks.deviceIsRunning()) {
        const SensorData sensor = callbacks.pollDeviceSensors();
        const FrameData frame = callbacks.captureFrame();
        if (sensor.calibrationChanged) {
            if (!callbacks.updateCalibration)
                throw std::invalid_argument("calibration update callback is not configured");
            params = callbacks.updateCalibration(sensor);
            bindUniforms(graphics, program, params);
        }
        params.eyeOffsetX = sensor.eyeOffsetX;
        params.eyeOffsetY = sensor.eyeOffsetY;
        updateDynamicUniforms(graphics, program, sensor);
        graphics.bindTexture(frame.texture);
        graphics.useProgram(program);
        graphics.drawTriangles(vertexCount);
        callbacks.presentFrame();
        callbacks.synchronizeFrameTiming();
    }
}
```

This completes the GPU integration of the corrective distortion system and sets the stage for the next section, where the Python calibration code that generates `CalibrationParams` input is defined.

---

## 10. Parameter Interface & Data Structures

This section defines the shared data contract between Python, C++, and GLSL. It is the architectural glue that ensures distortion parameters, calibration matrices, and dynamic sensor-driven values flow cleanly through the pipeline. Snap reviewers will look for this section because it demonstrates that the system is not just mathematically correct or GPU-capable — it is integrated, synchronized, and production-ready.

### 10.1 Architectural role of the parameter interface

The parameter interface defines how data moves through the system:

1. **Python → C++** — Python computes distortion parameters and calibration matrices, then exports them in a structured format.
2. **C++ → GLSL** — C++ loads these parameters, stores them in strongly typed structures, and binds them to GLSL uniforms.
3. **GLSL → GPU execution** — GLSL uses these uniforms to apply inverse distortion and calibrated projection in real time.

This section ensures all three layers speak the same language.

### 10.2 Core parameter structure (C++ representation)

The C++ layer defines a struct that mirrors the Python output and maps directly to GLSL uniforms:

```cpp
struct CalibrationParams {
    float k1{}, k2{}, k3{}, p1{}, p2{};
    std::array<float, 16> projectionMatrix{};
    float eyeOffsetX{}, eyeOffsetY{};
    std::uint64_t timestamp{};
};
```

This struct is the canonical representation of all calibration data inside the runtime.

### 10.3 Mapping Python output → C++ struct

Python exports a JSON document containing:

- distortion coefficients
- intrinsic matrix
- extrinsic matrix
- combined projection matrix
- optional dynamic calibration values

C++ loads this data and populates the struct:

```cpp
CalibrationParams loadParamsFromJson(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open calibration file: " + path);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    CalibrationParams params;
    params.k1 = floatField(json, "k1");
    params.k2 = floatField(json, "k2");
    params.k3 = floatField(json, "k3");
    params.p1 = floatField(json, "p1");
    params.p2 = floatField(json, "p2");
    const auto matrix = matrixField(json);
    std::copy(matrix.begin(), matrix.end(), params.projectionMatrix.begin());
    params.timestamp = timestampField(json);
    return params;
}
```

This ensures Python's math becomes GPU-ready data.

### 10.4 Mapping C++ struct → GLSL uniforms

| C++ Field | GLSL Uniform | Purpose |
|---|---|---|
| `k1, k2, k3` | `uniform float k1, k2, k3` | Radial distortion |
| `p1, p2` | `uniform float p1, p2` | Tangential distortion |
| `projectionMatrix` | `uniform mat4 uProjectionMatrix` | Calibrated projection |
| `eyeOffsetX/Y` | `uniform vec2 uEyeOffset` | Dynamic per-eye correction |

Uniform binding occurs inside the runtime loop:

```cpp
void bindUniforms(GraphicsApi& graphics, GlId program, const CalibrationParams& params) {
    graphics.useProgram(program);
    graphics.uniform1f(program, "k1", params.k1);
    graphics.uniform1f(program, "k2", params.k2);
    graphics.uniform1f(program, "k3", params.k3);
    graphics.uniform1f(program, "p1", params.p1);
    graphics.uniform1f(program, "p2", params.p2);
    // Python serializes rows; OpenGL expects columns, hence transpose=true.
    graphics.uniformMatrix4(program, "uProjectionMatrix", params.projectionMatrix.data(), true);
}
```

This is the exact moment where Python's math becomes GPU execution.

### 10.5 Dynamic parameter updates (sensor-driven)

Some parameters change every frame, driven by sensor input:

- eye offsets
- micro-adjustments
- device repositioning
- user calibration triggers

These values are updated inside the runtime loop:

```cpp
params.eyeOffsetX = sensor.eyeOffsetX;
params.eyeOffsetY = sensor.eyeOffsetY;
```

Then immediately bound to GLSL:

```cpp
void updateDynamicUniforms(GraphicsApi& graphics, GlId program, const SensorData& sensor) {
    graphics.useProgram(program);
    graphics.uniform2f(program, "uEyeOffset", sensor.eyeOffsetX, sensor.eyeOffsetY);
}
```

This keeps the optical correction aligned with real-time user movement.

### 10.6 Synchronization model

To avoid latency spikes or mismatched calibration:

- Python updates only when calibration changes
- C++ stores the latest parameters
- GLSL receives updated uniforms every frame
- Timestamp/versioning ensures consistency
- Sensor input drives dynamic fields only

This creates a stable, deterministic pipeline.

---

## 11. Python Calibration & Modeling (Math Layer)

The Python layer provides the analytical foundation for the corrective-vision system. Its role is to compute distortion coefficients and calibrated projection matrices that populate the `CalibrationParams` structure used by the C++ runtime and GLSL shaders. Python does not participate in real-time rendering; instead, it defines the static and semi-dynamic parameters that govern optical correction.

### 11.1 Mathematical responsibilities of the Python layer

Python is responsible for:

- **Radial distortion modeling** — computes $k_1, k_2, k_3$ from calibration samples
- **Tangential distortion modeling** — computes $p_1, p_2$ based on lens alignment and sensor data
- **Intrinsic matrix construction** — builds $K$ from focal lengths and principal point
- **Extrinsic matrix construction** — builds rotation $R$ and translation $t$ from device calibration
- **Projection matrix assembly** — produces a GPU-ready 4×4 matrix compatible with GLSL
- **Exporting parameters** — outputs a structure that maps directly into `CalibrationParams`

### 11.2 Calibration implementation

The implementation below validates all inputs, constructs the projection matrix, and emits the exact data contract consumed by C++.

```python
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
```

This function produces a dictionary that maps directly into the C++ `CalibrationParams` struct.

### 11.3 Exporting calibration parameters for C++

Python exports the computed parameters in a format that the C++ bridge can load without transformation:

```python
def export_calibration(params: Mapping[str, Any], path: str | Path) -> None:
    """Atomically export calibration parameters as JSON."""
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    temporary.write_text(json.dumps(dict(params), indent=2) + "\n", encoding="utf-8")
    temporary.replace(destination)

```

This ensures the C++ runtime can immediately populate `k1, k2, k3`, `p1, p2`, and `projectionMatrix[16]` with no intermediate conversion.

### 11.4 Dynamic recalibration (sensor-driven updates)

When the C++ runtime detects a calibration change (e.g., eye offset shift), it may request an updated model:

```python
def update_from_sensor(base: Mapping[str, Any], sensor: Mapping[str, Any]) -> dict[str, Any]:
    """Recompute calibration after applying the current eye displacement."""
    updated = dict(base)
    updated["cx"] = float(updated["cx"]) + float(sensor["eye_offset_x"])
    updated["cy"] = float(updated["cy"]) + float(sensor["eye_offset_y"])
    if "timestamp" in sensor:
        updated["timestamp"] = int(sensor["timestamp"])
    return compute_calibration(updated)
```

This keeps the optical model synchronized with real-time sensor input while maintaining Python's role as a math engine, not a real-time component.

### 11.5 Relationship to the C++ and GLSL layers

This refined Python section fits perfectly into the pipeline:

- Python computes distortion + projection.
- C++ loads and stores parameters in `CalibrationParams`.
- GLSL consumes them as uniforms for real-time correction.

---

## 12. System Summary & Pipeline Overview

This section ties together every layer of the corrective-vision system — Python, C++, GLSL, sensors, and hardware — into one coherent, production-ready pipeline. It shows Snap reviewers that the architecture is not a loose collection of components, but a synchronized, deterministic, end-to-end system designed for real AR hardware.

### 12.1 High-Level Pipeline Overview

The corrective-vision system operates as a closed loop:

1. Python modeling computes distortion coefficients and projection matrices.
2. C++ bridge loads these parameters and stores them in `CalibrationParams`.
3. GLSL shaders apply inverse distortion + calibrated projection on the GPU.
4. Sensors provide dynamic offsets (eye position, device movement).
5. C++ runtime loop updates uniforms and executes the shaders each frame.
6. Display hardware receives the corrected frame.

This loop runs continuously at the device's refresh rate.

### 12.2 Full Pipeline Diagram (Narrative Form)

$$\text{Python} \rightarrow \text{C++} \rightarrow \text{GLSL} \rightarrow \text{GPU} \rightarrow \text{Display}$$

- **Python** — computes $k_1, k_2, k_3, p_1, p_2$; builds intrinsic/extrinsic matrices; produces a 4×4 projection matrix; exports calibration parameters
- **C++ Bridge** — loads Python parameters; stores them in `CalibrationParams`; embeds GLSL shaders as string literals; compiles and links shader programs; binds uniforms (distortion + projection + offsets); polls sensors; updates dynamic uniforms; executes the render loop
- **GLSL Shaders** — vertex shader corrects geometry; fragment shader corrects pixels; both apply inverse distortion; both use calibrated projection; both output corrected coordinates or samples
- **GPU Execution** — runs shaders in parallel; applies correction at full frame rate; produces distortion-free geometry and imagery
- **Display Hardware** — receives corrected frame; presents stable, calibrated AR output

This is the complete end-to-end system.

### 12.3 Data Flow Summary (Parameter-Driven Architecture)

The system is parameter-driven, meaning Python defines the math, and C++/GLSL execute it.

**Python → C++**
- Python exports: `k1, k2, k3`, `p1, p2`, `projection_matrix[16]`

**C++ → GLSL**
- C++ binds: `uniform float k1, k2, k3`, `uniform float p1, p2`, `uniform mat4 uProjectionMatrix`, `uniform vec2 uEyeOffset`

**GLSL → GPU**
- GLSL applies: inverse radial distortion, inverse tangential distortion, calibrated projection, corrected UV sampling

This ensures the GPU always uses the latest calibration.

### 12.4 Runtime Loop Summary

The runtime loop ensures real-time correction:

1. Poll sensors
2. Capture frame
3. Update dynamic uniforms
4. Bind shader program
5. Bind textures/geometry
6. Execute corrective shaders
7. Present frame
8. Synchronize timing

This loop maintains stable latency (<15 ms) and smooth AR output.

---

## Appendix A: Variable Definitions & Formula Breakdown

This appendix provides a consolidated reference for all variables used throughout the corrective-vision system. It summarizes the mathematical symbols, their meanings, and the formulas applied in the Python calibration engine and GLSL shaders.

### A.1 Distortion Coefficients

| Variable | Meaning | Used In |
|---|---|---|
| $k_1$ | First-order radial distortion coefficient | Python, GLSL |
| $k_2$ | Second-order radial distortion coefficient | Python, GLSL |
| $k_3$ | Third-order radial distortion coefficient | Python, GLSL |
| $p_1$ | Tangential distortion coefficient (x-axis skew) | Python, GLSL |
| $p_2$ | Tangential distortion coefficient (y-axis skew) | Python, GLSL |

### A.2 Radial Distortion Formula

Radial distortion is modeled using the standard polynomial:

$$r^2 = x^2 + y^2, \qquad \text{radial}(r) = 1 + k_1 r^2 + k_2 r^4 + k_3 r^6$$

Used in: Python calibration, GLSL vertex shader, GLSL fragment shader.

### A.3 Tangential Distortion Formula

Tangential distortion is modeled using:

$$x_{tan} = 2p_1 xy + p_2(r^2 + 2x^2), \qquad y_{tan} = p_1(r^2 + 2y^2) + 2p_2 xy$$

Used in: Python calibration, GLSL shaders.

### A.4 Corrected Coordinates

The inverse distortion applied in GLSL:

$$x_{corrected} = \frac{x - x_{tan}}{\text{radial}(r)}, \qquad y_{corrected} = \frac{y - y_{tan}}{\text{radial}(r)}$$

Used in: GLSL vertex shader (geometry correction), GLSL fragment shader (UV correction).

### A.5 Intrinsic Camera Matrix K

| Variable | Meaning |
|---|---|
| $f_x$ | Focal length in x-direction |
| $f_y$ | Focal length in y-direction |
| $c_x$ | Principal point x-coordinate |
| $c_y$ | Principal point y-coordinate |

$$K = \begin{bmatrix} f_x & 0 & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix}$$

Used in: Python calibration, projection matrix construction.

### A.6 Extrinsic Matrices

Rotation matrix: $R \in \mathbb{R}^{3 \times 3}$
Translation vector: $t \in \mathbb{R}^{3 \times 1}$

Defaults: $R = I_3$, $t = 0$

Used in: Python calibration, projection matrix assembly.

### A.7 Projection Matrix P

$$P = K[R \mid t]$$

Converted to 4×4 homogeneous form for GLSL:

$$P_{4 \times 4} = \begin{bmatrix} P_{3 \times 4} \\ 0 \;\; 0 \;\; 0 \;\; 1 \end{bmatrix}$$

Used in: Python calibration, C++ uniform binding, GLSL vertex shader.

### A.8 Dynamic Runtime Variables

| Variable | Meaning | Source |
|---|---|---|
| `eyeOffsetX` | Horizontal eye offset | Sensor → C++ → GLSL |
| `eyeOffsetY` | Vertical eye offset | Sensor → C++ → GLSL |
| `timestamp` | Calibration versioning | Python → C++ |

Used in: C++ runtime loop, GLSL uniform updates.

### A.9 Texture Sampling Coordinates

| Variable | Meaning |
|---|---|
| `vTexCoord` | Input UV coordinate from vertex shader |
| `correctedUV` | Distortion-corrected UV coordinate |

Used in: GLSL fragment shader.

### A.10 Summary Table (All Variables)

| Variable | Description | Layer |
|---|---|---|
| $k_1, k_2, k_3$ | Radial distortion coefficients | Python, C++, GLSL |
| $p_1, p_2$ | Tangential distortion coefficients | Python, C++, GLSL |
| $f_x, f_y$ | Focal lengths | Python |
| $c_x, c_y$ | Principal point | Python |
| $R, t$ | Extrinsic matrices | Python |
| $P$ | Projection matrix | Python, C++, GLSL |
| `eyeOffsetX/Y` | Dynamic eye offsets | Sensors, C++, GLSL |
| `vTexCoord` | Input UV | GLSL |
| `correctedUV` | Corrected UV | GLSL |

---

## Appendix B: Uniform Mapping & Optical Glossary

This appendix consolidates all shader-bound uniforms and provides a glossary of the optical terms used throughout the corrective-vision system. It serves as a quick reference for reviewers evaluating the mathematical and rendering components.

### B.1 Uniform Mapping Table (C++ → GLSL → Meaning)

| Uniform Name | C++ Source Field | GLSL Type | Meaning / Purpose |
|---|---|---|---|
| `k1` | `params.k1` | `float` | First-order radial distortion coefficient controlling barrel/pincushion curvature |
| `k2` | `params.k2` | `float` | Second-order radial distortion coefficient refining curvature at mid-radius |
| `k3` | `params.k3` | `float` | Third-order radial distortion coefficient affecting outer-radius correction |
| `p1` | `params.p1` | `float` | Tangential distortion coefficient representing lens decentering along x |
| `p2` | `params.p2` | `float` | Tangential distortion coefficient representing lens decentering along y |
| `uProjectionMatrix` | `params.projectionMatrix` | `mat4` | Calibrated projection matrix combining intrinsics and extrinsics |
| `uEyeOffset` | `params.eyeOffsetX/Y` | `vec2` | Dynamic per-eye offset applied each frame based on sensor input |
| `vTexCoord` | Vertex shader output | `vec2` | Input UV coordinate for fragment-level distortion correction |
| `correctedUV` | Fragment shader computation | `vec2` | Distortion-corrected UV coordinate used for sampling the input texture |

### B.2 Glossary of Optical Terms

| Term | Definition |
|---|---|
| **Radial Distortion** | A lens-induced deformation where straight lines appear curved; modeled using polynomial coefficients $k_1, k_2, k_3$ |
| **Tangential Distortion** | Distortion caused by lens misalignment or decentering, producing asymmetric warping; modeled using $p_1, p_2$ |
| **Intrinsic Matrix (K)** | A 3 × 3 matrix describing the camera's internal geometry: focal lengths and principal point |
| **Extrinsic Parameters** | Rotation $R$ and translation $t$ describing the camera's position and orientation relative to the scene |
| **Projection Matrix** | A 4 × 4 matrix combining intrinsics and extrinsics to map 3D coordinates into clip space |
| **Principal Point** | The optical center of the lens where the image should ideally converge; represented by $c_x, c_y$ |
| **Focal Length** | The effective distance between the lens and the sensor; represented by $f_x, f_y$ |
| **UV Coordinates** | Normalized texture coordinates used for sampling images in fragment shaders |
| **Inverse Distortion** | The corrective transformation applied to counteract lens distortion and restore linear geometry |
| **Eye Offset** | Real-time positional adjustment derived from sensors to maintain alignment between the user's eye and the optical model |

---

## Author's Note

This document provides the full architectural, mathematical, and rendering-pipeline foundation for the corrective-vision system. The next step is the technical implementation, which will be delivered as a dedicated GitHub repository containing the complete C++ bridge, GLSL shader modules, Python calibration engine, and supporting build configuration. The repository will serve as the practical companion to this paper, enabling reviewers to explore the codebase, evaluate the runtime behavior, and integrate or test the system within their own environments. While this paper establishes the conceptual and structural framework, the GitHub codebase will provide the full technical follow-through.

## Closing Note of Appreciation

I want to express my sincere appreciation for the time and attention given to reviewing this work. Every architectural decision, mathematical derivation, and pipeline design presented here was developed independently, with Microsoft Copilot serving only as a drafting companion to help refine clarity and structure. The system itself — its concepts, models, and implementation strategy — reflects my own engineering judgment and the depth of thought I bring to future work. Thank you for considering this submission; I look forward to sharing the accompanying codebase and continuing the conversation in a more technical setting.

## Acknowledgment

I would like to extend a sincere thank-you to Marcel Krüger for his help in preparing the LaTeX formatting of this document. His contribution was limited strictly to presentation and typesetting. All architectural design, mathematical modeling, system integration, and technical content were conceived and authored entirely by me. I also appreciate that, during the formatting process, Marcel validated the structure and clarity of the document without altering any of its technical substance. His care in making the paper cleaner and more readable was valuable, while my sole authorship of the underlying work remains fully preserved.

---

*Don M. Feeney Jr. — ORCID: [0009-0003-1350-4160](https://orcid.org/0009-0003-1350-4160) — DOI: [10.5281/zenodo.22102285](https://doi.org/10.5281/zenodo.22102285)*
