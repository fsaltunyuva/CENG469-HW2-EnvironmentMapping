#version 430

layout(location = 2) in vec2 fUV;
out vec4 outColor;

uniform sampler2D hdrsampler;

uniform float uMiddleGray;
uniform float uLWhite;

void main() {
    vec4 texSample = texture(hdrsampler, fUV);
    vec3 hdrColor = texSample.rgb;

    float avgLogLum = textureLod(hdrsampler, vec2(0.5, 0.5), 20.0).a; // to sample the average log luminance from the mipmap level
    float avgLum = exp(avgLogLum);

    float exposure = uMiddleGray / (avgLum + 0.001);
    vec3 result = hdrColor * exposure;
    
    float LWhite2 = uLWhite * uLWhite;
    result = (result * (1.0 + (result / LWhite2))) / (1.0 + result); // Reinhard

    // Gamma correction
    outColor = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}