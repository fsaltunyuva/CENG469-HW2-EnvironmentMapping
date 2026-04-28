#version 430
/*
	File Name	: color.vert
	Author		: Bora Yalciner
	Description	:

		Basic fragment shader that just outputs
		color to the FBO
*/

// Definitions
// These locations must match between vertex/fragment shaders
#define IN_UV		layout(location = 0)
#define IN_NORMAL	layout(location = 1)
#define IN_WORLD_POS layout(location = 2)
#define IN_HEIGHT	layout(location = 3)

// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)

// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)

// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in IN_HEIGHT float fHeight;
in IN_WORLD_POS vec3 fWorldPos;

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;

// Uniforms
U_MODE uniform uint uMode;
uniform vec3 uViewPos;

// Texture samplers
layout(binding = 0) uniform sampler2D tGrass;
layout(binding = 1) uniform sampler2D tRock;
layout(binding = 2) uniform sampler2D tSnow;
layout(binding = 3) uniform sampler2D tGrassRough;
layout(binding = 4) uniform sampler2D tRockRough;
layout(binding = 5) uniform sampler2D tSnowRough;
layout(binding = 6) uniform sampler2D tSkybox;

// same as in water shader for consistent spherical mapping
const float PI = 3.1415926;

vec2 SampleSphericalMap(vec3 v) {
	float phi = atan(v.z, v.x);
	float theta = acos(clamp(v.y, -1.0, 1.0));
	float u = (phi + PI) / (2.0 * PI); // converting from [-PI, PI] to [0, 1]
	float v_coord = 1.0 - (theta / PI); // converting from [0, PI] to [1, 0]
	return vec2(u, v_coord);
}

vec3 getAlbedo(float h) {
	if (h < 100.0)
		return vec3(0.1, 0.8, 0.1); // bright green for low altitudes

	if (h < 500.0)
		return vec3(0.0, 0.4, 0.0); // darker green for higher altitudes

	if (h < 1500.0)
		return vec3(0.5, 0.3, 0.1); // brown for mountain areas

	return vec3(1.0, 1.0, 1.0); // white for high altitudes
}

void main(void)
{
	vec3 albedo = getAlbedo(fHeight);
	vec3 normal = normalize(fNormal);

	uint mode = uMode;
	switch(mode)
	{
		// Lambertian
		case 0:
		{
			vec3 N = normalize(fNormal);
			vec2 uv = fUV * 1.0; //tiling

			float rockW = smoothstep(300.0, 1200.0, fHeight);
			float snowW = smoothstep(1800.0, 2500.0, fHeight);

			// more slope more rock
			float slopeW = 1.0 - smoothstep(0.65, 0.95, N.y);

			vec3 colorMix = mix(texture(tGrass, uv).rgb, texture(tRock, uv).rgb, rockW);
			colorMix = mix(colorMix, texture(tRock, uv).rgb, slopeW);
			colorMix = mix(colorMix, texture(tSnow, uv).rgb, snowW);

			float roughMix = mix(texture(tGrassRough, uv).r, texture(tRockRough, uv).r, rockW);
			roughMix = mix(roughMix, texture(tRockRough, uv).r, slopeW);
			roughMix = mix(roughMix, texture(tSnowRough, uv).r, snowW);

			vec3 L = normalize(vec3(0.5, 1.0, 0.5));

			vec3 lightColor = texture(tSkybox, SampleSphericalMap(L)).rgb; // using skybox color as light color
			vec3 V = normalize(uViewPos - fWorldPos);
			vec3 H = normalize(L + V);

			float diff = max(dot(N, L), 0.0);

			float spec = pow(max(dot(N, H), 0.0), (1.0 - roughMix) * 128.0); // dynamic specuar exponent based on roughness

			vec3 ambient = textureLod(tSkybox, SampleSphericalMap(N), 8.0).rgb * 0.1; // ambient from skybox with some scaling

			vec3 finalColor = (colorMix * (lightColor * diff + ambient)) + (spec * lightColor * 0.3); // specular contribution is scaled down a bit

			float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
			float logL = log(max(luminance, 0.0001));

			fboColor = vec4(finalColor, logL);
			break;
		}
		// Albedo (from HW1)
		case 1:
		{
			fboColor = vec4(albedo, 1.0);
			break;
		}
		// Surface Normals (from HW1)
		case 2:{
			// [-1, 1] to [0, 1]
			fboColor = vec4(normal * 0.5 + 0.5, 1.0);
			break;
		}

		// If mode is wrong, go white
		default: fboColor = vec4(1); break;
	}
}