#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace specs {

inline constexpr unsigned versionMajor = 1;
inline constexpr unsigned versionMinor = 0;
inline constexpr unsigned versionPatch = 0;
inline constexpr const char* versionString = "1.0.0";

using GlId = unsigned int;

struct CalibrationParams {
    // Brown-Conrady radial (k) and tangential (p) distortion coefficients.
    float k1{}, k2{}, k3{}, p1{}, p2{};
    // Row-major at the interchange boundary; bindUniforms transposes it for OpenGL.
    std::array<float, 16> projectionMatrix{};
    float eyeOffsetX{}, eyeOffsetY{};
    std::uint64_t timestamp{};
};

struct SensorData {
    float eyeOffsetX{}, eyeOffsetY{};
    bool calibrationChanged{};
};

struct FrameData { GlId texture{}; };

class GraphicsApi {
public:
    virtual ~GraphicsApi() = default;
    virtual GlId compileShader(unsigned type, const std::string& source) = 0;
    virtual GlId linkProgram(GlId vertex, GlId fragment) = 0;
    virtual void deleteShader(GlId shader) = 0;
    virtual void useProgram(GlId program) = 0;
    virtual void uniform1f(GlId program, const char* name, float value) = 0;
    virtual void uniform2f(GlId program, const char* name, float x, float y) = 0;
    virtual void uniformMatrix4(GlId program, const char* name,
                                const float* value, bool transpose) = 0;
    virtual void bindTexture(GlId texture) = 0;
    virtual void drawTriangles(int vertexCount) = 0;
};

struct PipelineCallbacks {
    // These hooks isolate the deterministic correction loop from a particular
    // device SDK, window system, and frame-present implementation.
    std::function<bool()> deviceIsRunning;
    std::function<SensorData()> pollDeviceSensors;
    std::function<FrameData()> captureFrame;
    std::function<CalibrationParams(const SensorData&)> updateCalibration;
    std::function<void()> presentFrame;
    std::function<void()> synchronizeFrameTiming;
};

CalibrationParams loadParamsFromJson(const std::string& path);
// Shader objects are deleted after linking (and on failure); ownership of the
// returned program remains with the GraphicsApi implementation/caller.
GlId createCorrectiveShaderProgram(GraphicsApi& graphics,
                                   const std::string& vertexSource,
                                   const std::string& fragmentSource);
void bindUniforms(GraphicsApi& graphics, GlId program, const CalibrationParams& params);
void updateDynamicUniforms(GraphicsApi& graphics, GlId program, const SensorData& sensor);
// Runs until deviceIsRunning returns false. Static calibration is rebound only
// when the sensor reports a change, while eye offsets are uploaded every frame.
void runCorrectivePipeline(GraphicsApi& graphics, GlId program, CalibrationParams params,
                           const PipelineCallbacks& callbacks, int vertexCount = 6);

extern const char* correctiveVertexShaderSource;
extern const char* correctiveFragmentShaderSource;

}  // namespace specs
