#include "optical_compensation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <regex>
#include <stdexcept>
#include <vector>

namespace specs {
namespace {

double numberField(const std::string& json, const std::string& name, bool optional = false) {
    const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*"
                             "([-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        if (optional) return 0.0;
        throw std::runtime_error("missing or invalid JSON field: " + name);
    }
    const double value = std::stod(match[1].str());
    if (!std::isfinite(value)) throw std::runtime_error("non-finite JSON field: " + name);
    return value;
}

std::vector<float> matrixField(const std::string& json) {
    const std::regex field("\\\"projection_matrix\\\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch match;
    if (!std::regex_search(json, match, field))
        throw std::runtime_error("missing projection_matrix JSON field");
    const std::regex number("[-+]?(?:[0-9]+\\.?[0-9]*|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?");
    std::vector<float> values;
    for (auto it = std::sregex_iterator(match[1].first, match[1].second, number);
         it != std::sregex_iterator(); ++it) {
        values.push_back(std::stof(it->str()));
    }
    if (values.size() != 16) throw std::runtime_error("projection_matrix must contain 16 numbers");
    return values;
}

}  // namespace

CalibrationParams loadParamsFromJson(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open calibration file: " + path);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    CalibrationParams params;
    params.k1 = static_cast<float>(numberField(json, "k1"));
    params.k2 = static_cast<float>(numberField(json, "k2"));
    params.k3 = static_cast<float>(numberField(json, "k3"));
    params.p1 = static_cast<float>(numberField(json, "p1"));
    params.p2 = static_cast<float>(numberField(json, "p2"));
    const auto matrix = matrixField(json);
    std::copy(matrix.begin(), matrix.end(), params.projectionMatrix.begin());
    const double timestamp = numberField(json, "timestamp", true);
    if (timestamp < 0 || timestamp > static_cast<double>(std::numeric_limits<std::uint64_t>::max()))
        throw std::runtime_error("timestamp is outside the uint64 range");
    params.timestamp = static_cast<std::uint64_t>(timestamp);
    return params;
}

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

void updateDynamicUniforms(GraphicsApi& graphics, GlId program, const SensorData& sensor) {
    graphics.useProgram(program);
    graphics.uniform2f(program, "uEyeOffset", sensor.eyeOffsetX, sensor.eyeOffsetY);
}

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

}  // namespace specs
