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
#define IN_COLOR	layout(location = 2)
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

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;

// Uniforms
U_MODE uniform uint uMode;

// Textures
uniform T_ALBEDO sampler2D tAlbedo;

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
			vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
			float diff = max(dot(normal, lightDir), 0.2); // 0.2 ambient

			vec3 color = albedo * diff;

			float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)); // sRGB
			float logLuminance = log(max(luminance, 0.0001));

//			color *= 20; // to test hdr

			fboColor = vec4(color, logLuminance); // passing logLuminance to alpha value
			break;
		}
		// Albedo
		case 1:
		{
			fboColor = vec4(albedo, 1.0);
			break;
		}
		// Surface Normals
		case 2:{
			// [-1, 1] to [0, 1]
			fboColor = vec4(normal * 0.5 + 0.5, 1.0);
			break;
		}

		// If mode is wrong, go white
		default: fboColor = vec4(1); break;
	}
}