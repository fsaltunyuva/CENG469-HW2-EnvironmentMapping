#version 430

layout(location = 0) in vec3 vPos;

out gl_PerVertex {vec4 gl_Position;};
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fWorldPos;

layout(location = 0) uniform mat4 uModel;
layout(location = 1) uniform mat4 uView;
layout(location = 2) uniform mat4 uProj;
layout(location = 4) uniform float uTime;
layout(location = 5) uniform float uWaterLevel;
layout(location = 6) uniform float uAmplitude;
layout(location = 7) uniform float uFrequency;
layout(location = 8) uniform float uPhase;

void main() {
    // y = A * cos(F*x + W*t)
    float height = uAmplitude * cos(uFrequency * vPos.x + uPhase * uTime);
    vec3 animatedPos = vec3(vPos.x, uWaterLevel + height, vPos.z);

    // Derivative
    // dy/dx = -A * F * sin(F*x + W*t)
    float dy_dx = -uAmplitude * uFrequency * sin(uFrequency * vPos.x + uPhase * uTime);

    // Cross products of the tangent vectors gives the normal
    fNormal = normalize(vec3(-dy_dx, 1.0, 0.0));

    fWorldPos = animatedPos;
    gl_Position = uProj * uView * uModel * vec4(animatedPos, 1.0);
}