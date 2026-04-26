#version 430

in vec3 vDir;

layout(location = 0) out vec4 fboColor;

layout(binding = 0) uniform sampler2D tSkybox;

const float PI = 3.14159265359;

vec2 SampleSphericalMap(vec3 v)
{
    float phi = atan(v.z, v.x);
    float theta = acos(v.y);

    float u = (phi + PI) / (2.0 * PI);
    float v_coord = 1.0 - (theta / PI);

    return vec2(u, v_coord);
}

void main()
{
    vec3 dir = normalize(vDir);
    vec2 uv = SampleSphericalMap(dir);

    vec3 envColor = textureLod(tSkybox, uv, 0.0).rgb;

    float luminance = dot(envColor, vec3(0.2126, 0.7152, 0.0722));
    float logLuminance = log(max(luminance, 0.0001));

    fboColor = vec4(envColor, logLuminance);
}