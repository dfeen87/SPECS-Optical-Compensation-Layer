#include "optical_compensation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("test failure: " + message);
}

template <typename Exception, typename Function>
void requireThrows(Function operation, const std::string& message) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("test failure: expected exception: " + message);
}

const std::string validJson = R"({"k1":0.1,"k2":0.2,"k3":0.3,"p1":0.4,"p2":0.5,
  "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"timestamp":12})";

void writeFile(const std::string& path, const std::string& contents) {
    std::ofstream output(path);
    require(static_cast<bool>(output), "test calibration file opens");
    output << contents;
    require(static_cast<bool>(output), "test calibration file writes");
}

void expectInvalidCalibration(const std::string& path, const std::string& contents) {
    writeFile(path, contents);
    requireThrows<std::runtime_error>([&] { (void)specs::loadParamsFromJson(path); }, contents);
}

class FakeGraphics final : public specs::GraphicsApi {
public:
    unsigned compileShader(unsigned, const std::string& source) override {
        events.push_back("compile");
        if (throwCompileAt == ++compileCalls) throw std::runtime_error("compile failed");
        require(source.find("void main") != std::string::npos, "shader has main");
        return ++next;
    }
    unsigned linkProgram(unsigned, unsigned) override {
        events.push_back("link");
        if (throwLink) throw std::runtime_error("link failed");
        return 9;
    }
    void deleteShader(unsigned shader) override { deleted.push_back(shader); }
    void useProgram(unsigned program) override { used = program; ++graphicsMutations; }
    void uniform1f(unsigned, const char* name, float value) override {
        scalars[name] = value; ++graphicsMutations;
    }
    void uniform2f(unsigned, const char*, float x, float y) override {
        eyeX = x; eyeY = y; ++graphicsMutations;
    }
    void uniformMatrix4(unsigned, const char*, const float*, bool value) override {
        transpose = value; ++graphicsMutations;
    }
    void bindTexture(unsigned texture) override { bound = texture; events.push_back("bind"); }
    void drawTriangles(int count) override { drawn = count; events.push_back("draw"); }

    unsigned next{}, compileCalls{}, throwCompileAt{}, used{}, bound{}, graphicsMutations{};
    bool throwLink{}, transpose{};
    int drawn{};
    float eyeX{}, eyeY{};
    std::map<std::string, float> scalars;
    std::vector<unsigned> deleted;
    std::vector<std::string> events;
};

specs::CalibrationParams identityCalibration() {
    specs::CalibrationParams params;
    params.projectionMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    return params;
}

void testJsonBoundary(const std::string& path) {
    writeFile(path, validJson);
    const auto params = specs::loadParamsFromJson(path);
    require(params.timestamp == 12, "timestamp loads exactly");
    require(params.projectionMatrix[10] == 1, "matrix loads in row-major order");
    require(params.p2 == .5f, "coefficient loads");

    expectInvalidCalibration(path, R"({"k1":1e100,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})");
    expectInvalidCalibration(path, R"({"k1":0,"k1":1,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})");
    expectInvalidCalibration(path, R"({"wrapper":{"k1":0},"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})");
    expectInvalidCalibration(path, validJson + " trailing");
    expectInvalidCalibration(path, R"({"k1":0,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,oops,1]})");
    for (const std::string timestamp : {"-1", "1.5", "1e1", "18446744073709551616"}) {
        std::string malformed = validJson;
        malformed.replace(malformed.find("12}"), 2, timestamp);
        expectInvalidCalibration(path, malformed);
    }
}

void testShaderLifecycle() {
    FakeGraphics graphics;
    const auto program = specs::createCorrectiveShaderProgram(
        graphics, specs::correctiveVertexShaderSource, specs::correctiveFragmentShaderSource);
    require(program == 9 && graphics.deleted == std::vector<unsigned>({1, 2}),
            "successful link releases both shaders");

    FakeGraphics fragmentFailure;
    fragmentFailure.throwCompileAt = 2;
    requireThrows<std::runtime_error>([&] {
        specs::createCorrectiveShaderProgram(fragmentFailure, "void main(){}", "void main(){}");
    }, "fragment compilation failure");
    require(fragmentFailure.deleted == std::vector<unsigned>({1}),
            "fragment failure releases the vertex shader");

    FakeGraphics linkFailure;
    linkFailure.throwLink = true;
    requireThrows<std::runtime_error>([&] {
        specs::createCorrectiveShaderProgram(linkFailure, "void main(){}", "void main(){}");
    }, "link failure");
    require(linkFailure.deleted == std::vector<unsigned>({1, 2}),
            "link failure releases both shaders");
}

void testPipeline() {
    auto params = identityCalibration();
    params.p2 = .5f;
    FakeGraphics graphics;
    int frames = 0, presented = 0, synchronized = 0, updates = 0;
    specs::PipelineCallbacks callbacks;
    callbacks.deviceIsRunning = [&] { return frames++ < 1; };
    callbacks.pollDeviceSensors = [] { return specs::SensorData{.25f, -.5f, true}; };
    callbacks.captureFrame = [&] { graphics.events.push_back("capture"); return specs::FrameData{42}; };
    callbacks.updateCalibration = [&](const specs::SensorData&) { ++updates; return params; };
    callbacks.presentFrame = [&] { ++presented; graphics.events.push_back("present"); };
    callbacks.synchronizeFrameTiming = [&] { ++synchronized; graphics.events.push_back("sync"); };
    specs::runCorrectivePipeline(graphics, 9, params, callbacks);
    require(graphics.used == 9 && graphics.transpose, "program and matrix are bound");
    require(graphics.scalars["p2"] == .5f, "static uniform is bound");
    require(graphics.eyeX == .25f && graphics.eyeY == -.5f, "dynamic pose is bound");
    require(graphics.bound == 42 && graphics.drawn == 6, "captured frame is drawn");
    require(presented == 1 && synchronized == 1 && updates == 1, "frame callbacks run once");
    const auto capture = std::find(graphics.events.begin(), graphics.events.end(), "capture");
    const auto draw = std::find(graphics.events.begin(), graphics.events.end(), "draw");
    const auto present = std::find(graphics.events.begin(), graphics.events.end(), "present");
    const auto sync = std::find(graphics.events.begin(), graphics.events.end(), "sync");
    require(capture < draw && draw < present && present < sync, "frame ordering is preserved");
}

void testFailClosedValidation() {
    auto params = identityCalibration();
    params.k1 = std::numeric_limits<float>::quiet_NaN();
    FakeGraphics invalidInitial;
    specs::PipelineCallbacks stopped;
    stopped.deviceIsRunning = [] { return false; };
    stopped.pollDeviceSensors = [] { return specs::SensorData{}; };
    stopped.captureFrame = [] { return specs::FrameData{}; };
    stopped.presentFrame = [] {};
    stopped.synchronizeFrameTiming = [] {};
    requireThrows<std::invalid_argument>([&] {
        specs::runCorrectivePipeline(invalidInitial, 9, params, stopped);
    }, "invalid initial calibration");
    require(invalidInitial.graphicsMutations == 0, "invalid calibration causes no graphics mutation");

    params = identityCalibration();
    FakeGraphics invalidSensor;
    int captures = 0;
    auto callbacks = stopped;
    callbacks.deviceIsRunning = [calls = 0]() mutable { return calls++ == 0; };
    callbacks.pollDeviceSensors = [] {
        return specs::SensorData{std::numeric_limits<float>::infinity(), 0, false};
    };
    callbacks.captureFrame = [&] { ++captures; return specs::FrameData{1}; };
    requireThrows<std::invalid_argument>([&] {
        specs::runCorrectivePipeline(invalidSensor, 9, params, callbacks);
    }, "invalid sensor data");
    require(captures == 0 && invalidSensor.drawn == 0,
            "invalid sensor data is rejected before capture and draw");

    FakeGraphics invalidRefresh;
    callbacks.deviceIsRunning = [calls = 0]() mutable { return calls++ == 0; };
    callbacks.pollDeviceSensors = [] { return specs::SensorData{0, 0, true}; };
    callbacks.updateCalibration = [](const specs::SensorData&) {
        auto invalid = identityCalibration();
        invalid.projectionMatrix[3] = std::numeric_limits<float>::quiet_NaN();
        return invalid;
    };
    requireThrows<std::invalid_argument>([&] {
        specs::runCorrectivePipeline(invalidRefresh, 9, params, callbacks);
    }, "invalid refreshed calibration");
    require(captures == 0 && invalidRefresh.drawn == 0,
            "invalid refreshed calibration is rejected before capture and draw");

    callbacks = {};
    requireThrows<std::invalid_argument>([&] {
        specs::runCorrectivePipeline(invalidRefresh, 9, params, callbacks);
    }, "missing callbacks");
}

}  // namespace

int main() {
    static_assert(specs::versionMajor == 2 && specs::versionMinor == 0 &&
                  specs::versionPatch == 0);
    require(std::string(specs::versionString) == "2.0.0", "runtime version matches release");
    const std::string path = "test-calibration.json";
    try {
        testJsonBoundary(path);
        testShaderLifecycle();
        testPipeline();
        testFailClosedValidation();
        std::remove(path.c_str());
        return 0;
    } catch (...) {
        std::remove(path.c_str());
        throw;
    }
}
