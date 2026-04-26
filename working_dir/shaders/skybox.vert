#version 430

layout(location = 0) in vec4 vPos;

out vec3 vDir;

layout(location = 0) uniform mat4 uInvProj;
layout(location = 1) uniform mat4 uInvView;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(vPos.xy, 1.0, 1.0);

    // Compute view direction for the equirectangular map lookup
    vec4 clipPos = vec4(vPos.xy, -1.0, 1.0);
    vec4 viewPos = uInvProj * clipPos;
    viewPos.z = -1.0;
    viewPos.w = 0.0;
    
    vDir = (uInvView * viewPos).xyz;
}
