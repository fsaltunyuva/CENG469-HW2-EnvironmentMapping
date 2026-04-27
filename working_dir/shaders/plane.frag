#version 430

layout(location = 0) in vec2 fUV;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec3 fWorldPos;

layout(location = 0) out vec4 fboColor;

uniform sampler2D tAlbedo;
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

    if (uIsGlass) {
        vec3 V = normalize(uViewPos - fWorldPos);
        vec3 R = reflect(-V, normal);
        vec3 reflectColor = texture(tAlbedo, SampleSphericalMap(R)).rgb;
        fboColor = vec4(reflectColor, log(max(dot(reflectColor, vec3(0.2126, 0.7152, 0.0722)), 0.0001)));
    }
    else{
        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
        float diff = max(dot(normal, lightDir), 0.2);

        vec3 texCol = texture(tAlbedo, fUV).rgb;
        vec3 color = texCol * diff;

        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)); // sRGB
        float logLuminance = log(max(luminance, 0.0001));

        fboColor = vec4(color, logLuminance);
    }
}