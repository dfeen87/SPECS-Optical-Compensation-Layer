#include "optical_compensation.hpp"

namespace specs {

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

}  // namespace specs
