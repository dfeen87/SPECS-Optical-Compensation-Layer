#include "optical_compensation.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>

namespace specs {
namespace {

class CalibrationJsonParser {
public:
    explicit CalibrationJsonParser(const std::string& text) : text_(text) {}

    CalibrationParams parse() {
        CalibrationParams result;
        whitespace();
        expect('{');
        whitespace();
        if (consume('}')) fail("calibration object is empty");
        for (;;) {
            const std::string key = string();
            whitespace();
            expect(':');
            whitespace();
            if (!seen_.insert(key).second) fail("duplicate field: " + key);
            if (key == "k1") result.k1 = floatValue(key);
            else if (key == "k2") result.k2 = floatValue(key);
            else if (key == "k3") result.k3 = floatValue(key);
            else if (key == "p1") result.p1 = floatValue(key);
            else if (key == "p2") result.p2 = floatValue(key);
            else if (key == "projection_matrix") matrix(result.projectionMatrix);
            else if (key == "timestamp") result.timestamp = timestamp();
            else skipValue();
            whitespace();
            if (consume('}')) break;
            expect(',');
            whitespace();
        }
        whitespace();
        if (position_ != text_.size()) fail("trailing content");
        for (const char* required : {"k1", "k2", "k3", "p1", "p2", "projection_matrix"})
            if (!seen_.count(required)) fail(std::string("missing field: ") + required);
        return result;
    }

private:
    [[noreturn]] void fail(const std::string& reason) const {
        throw std::runtime_error("invalid calibration JSON: " + reason);
    }
    void whitespace() {
        while (position_ < text_.size() &&
               (text_[position_] == ' ' || text_[position_] == '\n' ||
                text_[position_] == '\r' || text_[position_] == '\t')) ++position_;
    }
    bool consume(char value) {
        if (position_ < text_.size() && text_[position_] == value) {
            ++position_;
            return true;
        }
        return false;
    }
    void expect(char value) {
        if (!consume(value)) fail(std::string("expected '") + value + "'");
    }
    std::string string() {
        expect('"');
        std::string result;
        while (position_ < text_.size()) {
            const char value = text_[position_++];
            if (value == '"') return result;
            if (static_cast<unsigned char>(value) < 0x20) fail("control character in string");
            if (value == '\\') {
                if (position_ >= text_.size()) fail("unterminated escape");
                const char escaped = text_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/') result += escaped;
                else if (escaped == 'b') result += '\b';
                else if (escaped == 'f') result += '\f';
                else if (escaped == 'n') result += '\n';
                else if (escaped == 'r') result += '\r';
                else if (escaped == 't') result += '\t';
                else fail("unsupported string escape");
            } else result += value;
        }
        fail("unterminated string");
    }
    std::string numberText() {
        const std::size_t begin = position_;
        consume('-');
        if (consume('0')) {
            if (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
                fail("leading zero in number");
        } else {
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
            if (digits == position_) fail("expected number");
        }
        if (consume('.')) {
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
            if (digits == position_) fail("invalid number fraction");
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-'))
                ++position_;
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
            if (digits == position_) fail("invalid number exponent");
        }
        return text_.substr(begin, position_ - begin);
    }
    float floatValue(const std::string& name) {
        const std::string token = numberText();
        double value{};
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size())
            fail("invalid number for " + name);
        if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
            fail(name + " is outside the native float range");
        return static_cast<float>(value);
    }
    std::uint64_t timestamp() {
        const std::string token = numberText();
        if (token.empty() || token.front() == '-' || token.find_first_of(".eE") != std::string::npos)
            fail("timestamp must be a uint64 integer");
        std::uint64_t result{};
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), result);
        if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size())
            fail("timestamp is outside the uint64 range");
        return result;
    }
    void matrix(std::array<float, 16>& result) {
        expect('[');
        whitespace();
        std::size_t count = 0;
        if (consume(']')) fail("projection_matrix must contain 16 numbers");
        for (;;) {
            if (count >= result.size()) fail("projection_matrix must contain 16 numbers");
            result[count++] = floatValue("projection_matrix");
            whitespace();
            if (consume(']')) break;
            expect(',');
            whitespace();
        }
        if (count != result.size()) fail("projection_matrix must contain 16 numbers");
    }
    void skipValue() {
        whitespace();
        if (position_ >= text_.size()) fail("missing value");
        if (text_[position_] == '"') { (void)string(); return; }
        if (text_[position_] == '{') {
            ++position_; whitespace();
            if (consume('}')) return;
            for (;;) {
                (void)string();
                whitespace();
                expect(':');
                skipValue();
                whitespace();
                if (consume('}')) return;
                expect(',');
                whitespace();
            }
        }
        if (text_[position_] == '[') {
            ++position_; whitespace();
            if (consume(']')) return;
            for (;;) { skipValue(); whitespace(); if (consume(']')) return; expect(','); whitespace(); }
        }
        for (const std::string literal : {"true", "false", "null"})
            if (text_.compare(position_, literal.size(), literal) == 0) {
                position_ += literal.size(); return;
            }
        (void)numberText();
    }

    const std::string& text_;
    std::size_t position_{};
    std::set<std::string> seen_;
};

void validateCalibration(const CalibrationParams& params) {
    const auto finite = [](float value) { return std::isfinite(value); };
    if (!finite(params.k1) || !finite(params.k2) || !finite(params.k3) ||
        !finite(params.p1) || !finite(params.p2) ||
        !std::all_of(params.projectionMatrix.begin(), params.projectionMatrix.end(), finite))
        throw std::invalid_argument("calibration values must be finite");
}

void validateSensor(const SensorData& sensor) {
    if (!std::isfinite(sensor.eyeOffsetX) || !std::isfinite(sensor.eyeOffsetY))
        throw std::invalid_argument("sensor eye offsets must be finite");
}

}  // namespace

CalibrationParams loadParamsFromJson(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open calibration file: " + path);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    if (input.bad()) throw std::runtime_error("cannot read calibration file: " + path);
    return CalibrationJsonParser(json).parse();
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
    validateCalibration(params);
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
    validateSensor(sensor);
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
        // Sample sensors before capture so the correction applied to this frame
        // reflects the freshest available eye pose and calibration state.
        const SensorData sensor = callbacks.pollDeviceSensors();
        validateSensor(sensor);
        if (sensor.calibrationChanged) {
            if (!callbacks.updateCalibration)
                throw std::invalid_argument("calibration update callback is not configured");
            CalibrationParams candidate = callbacks.updateCalibration(sensor);
            validateCalibration(candidate);
            bindUniforms(graphics, program, candidate);
            params = candidate;
        }
        // Validate and bind any replacement calibration before consuming a frame.
        const FrameData frame = callbacks.captureFrame();
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
