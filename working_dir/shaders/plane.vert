#version 430

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

out gl_PerVertex {vec4 gl_Position;};
layout(location = 0) out vec2 fUV;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fWorldPos;

layout(location = 0) uniform mat4 uModel;
layout(location = 1) uniform mat4 uView;
layout(location = 2) uniform mat4 uProjection;
layout(location = 3) uniform mat3 uNormalMatrix;

void main(void)
{
    fUV = vUV;
    fWorldPos = vec3(uModel * vec4(vPos, 1.0));
    fNormal = normalize(uNormalMatrix * vNormal);
    gl_Position = uProjection * uView * uModel * vec4(vPos, 1.0);
}