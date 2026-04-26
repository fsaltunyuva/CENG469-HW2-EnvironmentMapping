#version 430

layout(location = 0) in vec2 vPos;
layout(location = 2) in vec2 vUV;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 2) out vec2 fUV;

void main() {
    fUV = vUV;
    gl_Position = vec4(vPos, 0.0, 1.0);
}