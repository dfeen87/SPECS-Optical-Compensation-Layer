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
    // Evaluate distortion in centered [-1, 1] lens space, not texture space.
    vec2 centered = (vTexCoord - vec2(0.5)) * 2.0 + uEyeOffset;
    float x = centered.x;
    float y = centered.y;
    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float r6 = r4 * r2;
    // Guard the inverse mapping from singular coefficients near the lens edge.
    float radial = max(1.0 + k1 * r2 + k2 * r4 + k3 * r6, 0.000001);
    float xTangential = 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float yTangential = p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    vec2 corrected = vec2((x - xTangential) / radial,
                          (y - yTangential) / radial);
    vec2 correctedUV = (corrected - uEyeOffset) * 0.5 + vec2(0.5);
    fragColor = texture(uInputFrame, correctedUV);
}
