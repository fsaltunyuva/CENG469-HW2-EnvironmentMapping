#version 430

layout(location = 0) in vec2 fUV;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec3 fWorldPos;

layout(location = 0) out vec4 fboColor;

uniform sampler2D tAlbedo;
layout(binding = 1) uniform sampler2D tSkybox;
uniform vec3 uViewPos;
uniform bool uIsGlass;

// same as in water.frag
const float PI = 3.1415926;

vec2 SampleSphericalMap(vec3 v) {
    float phi = atan(v.z, v.x);
    float theta = acos(clamp(v.y, -1.0, 1.0));
    float u = (phi + PI) / (2.0 * PI); // converting from [-PI, PI] to [0, 1]
    float v_coord = 1.0 - (theta / PI); // converting from [0, PI] to [1, 0]
    return vec2(u, v_coord);
}

void main(void)
{
    vec3 normal = normalize(fNormal);
    vec3 V = normalize(uViewPos - fWorldPos);
    vec3 R = reflect(-V, normal);
    vec3 reflectColor = texture(tSkybox, SampleSphericalMap(R)).rgb;

    // I used mix() to blend the base color and the reflection to match with the shared demo video

    if (uIsGlass) {
        vec3 color = mix(vec3(0.05), reflectColor, 0.9); // 0.9 reflection and 0.1 base color

        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)); // sRGB
        float logLuminance = log(max(luminance, 0.0001));

        fboColor = vec4(color, logLuminance);
    }
    else {
        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
        float diff = max(dot(normal, lightDir), 0.2);

        vec3 texCol = texture(tAlbedo, fUV).rgb;
        vec3 color = mix(texCol * diff, reflectColor, 0.15); // 0.15 reflection and 0.85 base color

        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)); // sRGB
        float logLuminance = log(max(luminance, 0.0001));

        fboColor = vec4(color, logLuminance);
    }
}