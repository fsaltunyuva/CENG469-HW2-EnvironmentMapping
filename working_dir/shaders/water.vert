#version 430

layout(location = 0) in vec3 vPos;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fWorldPos;

uniform mat4 uModel, uView, uProj;
uniform float uTime;
uniform float uWaterLevel;

void main() {
    float A = 1.5; // amplitude
    float F = 0.003; // freq.
    float W = 0.6; // phase/frequency over time

    // y = A * cos(F*x + W*t)
    float height = A * cos(F * vPos.x + W * uTime);
    vec3 animatedPos = vec3(vPos.x, uWaterLevel + height, vPos.z);

    // Derivative
    // dy/dx = -A * F * sin(F*x + W*t)
    float dy_dx = -A * F * sin(F * vPos.x + W * uTime);

    // Cross products of the tangent vectors gives the normal
    fNormal = normalize(vec3(-dy_dx, 1.0, 0.0));

    fWorldPos = animatedPos;
    gl_Position = uProj * uView * uModel * vec4(animatedPos, 1.0);
}