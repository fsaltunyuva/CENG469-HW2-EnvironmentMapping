#version 430

layout(location = 0) in vec2 fUV;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec3 fWorldPos;

layout(location = 0) out vec4 fboColor;

void main(void)
{
    vec3 normal = normalize(fNormal);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff = max(dot(normal, lightDir), 0.2);

    vec3 color = vec3(0.5, 0.5, 0.5) * diff; // Base color with diffuse lighting

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)); // sRGB
    float logLuminance = log(max(luminance, 0.0001));

    fboColor = vec4(color, logLuminance);
}