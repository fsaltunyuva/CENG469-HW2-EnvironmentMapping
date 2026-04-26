#version 430

layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec3 fWorldPos;
layout(location = 0) out vec4 fboColor;

uniform vec3 uViewPos;
uniform sampler2D tSkybox;

const float PI = 3.1415926;

vec2 SampleSphericalMap(vec3 v) {
    float phi = atan(v.z, v.x);
    float theta = acos(clamp(v.y, -1.0, 1.0));
    float u = (phi + PI) / (2.0 * PI); // converting from [-PI, PI] to [0, 1]
    float v_coord = 1.0 - (theta / PI); // converting from [0, PI] to [1, 0]
    return vec2(u, v_coord);
}

void main() {
    vec3 V = normalize(uViewPos - fWorldPos);
    vec3 N = normalize(fNormal);
    N = faceforward(N, -V, N); // Look at camera

    vec3 R = reflect(-V, N);
    R.y = abs(R.y); // ensuring reflection looks upwards

    vec3 reflectionColor = textureLod(tSkybox, SampleSphericalMap(R), 0.0).rgb;

    vec3 waterBaseColor = vec3(0.01, 0.08, 0.15);
    vec3 finalColor = mix(waterBaseColor, reflectionColor, 0.5);

    float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    float logL = log(max(luminance, 0.0001));
    fboColor = vec4(finalColor, logL);
}