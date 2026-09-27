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
