#include "optical_compensation.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

void expectInvalidCalibration(const std::string& path, const std::string& contents) {
    std::ofstream(path) << contents;
    try {
        (void)specs::loadParamsFromJson(path);
        assert(false && "invalid calibration was accepted");
    } catch (const std::runtime_error&) {
    }
}

}  // namespace

class FakeGraphics final : public specs::GraphicsApi {
public:
    unsigned compileShader(unsigned, const std::string& source) override {
        assert(source.find("void main") != std::string::npos); return ++next;
    }
    unsigned linkProgram(unsigned, unsigned) override { return 9; }
    void deleteShader(unsigned) override { ++deleted; }
    void useProgram(unsigned program) override { used = program; }
    void uniform1f(unsigned, const char* name, float value) override { scalars[name] = value; }
    void uniform2f(unsigned, const char*, float x, float y) override { eyeX=x; eyeY=y; }
    void uniformMatrix4(unsigned, const char*, const float*, bool value) override { transpose=value; }
    void bindTexture(unsigned texture) override { bound=texture; }
    void drawTriangles(int count) override { drawn=count; }
    unsigned next{}, deleted{}, used{}, bound{}; int drawn{}; float eyeX{}, eyeY{};
    bool transpose{}; std::map<std::string,float> scalars;
};

int main() {
    const std::string path = "test-calibration.json";
    std::ofstream(path) << R"({"k1":0.1,"k2":0.2,"k3":0.3,"p1":0.4,"p2":0.5,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"timestamp":12})";
    auto params = specs::loadParamsFromJson(path);
    assert(params.timestamp == 12 && params.projectionMatrix[10] == 1);
    FakeGraphics graphics;
    auto program = specs::createCorrectiveShaderProgram(
        graphics, specs::correctiveVertexShaderSource, specs::correctiveFragmentShaderSource);
    int frames = 0, presented = 0, synchronized = 0;
    specs::PipelineCallbacks callbacks;
    callbacks.deviceIsRunning = [&] { return frames++ < 1; };
    callbacks.pollDeviceSensors = [] { return specs::SensorData{.25f, -.5f, true}; };
    callbacks.captureFrame = [] { return specs::FrameData{42}; };
    callbacks.updateCalibration = [=](const specs::SensorData&) { return params; };
    callbacks.presentFrame = [&] { ++presented; };
    callbacks.synchronizeFrameTiming = [&] { ++synchronized; };
    specs::runCorrectivePipeline(graphics, program, params, callbacks);
    assert(graphics.deleted == 2 && graphics.used == 9 && graphics.transpose);
    assert(graphics.scalars["p2"] == .5f && graphics.eyeX == .25f && graphics.eyeY == -.5f);
    assert(graphics.bound == 42 && graphics.drawn == 6 && presented == 1 && synchronized == 1);
    expectInvalidCalibration(path, R"({"k1":1e100,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})");
    expectInvalidCalibration(path, R"({"k1":0,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"timestamp":-1})");
    expectInvalidCalibration(path, R"({"k1":0,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"timestamp":1.5})");
    expectInvalidCalibration(path, R"({"k1":0,"k2":0,"k3":0,"p1":0,"p2":0,
      "projection_matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],
      "timestamp":18446744073709551616})");
    std::remove(path.c_str());
}
