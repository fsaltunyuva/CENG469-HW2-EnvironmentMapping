#version 430
/*
	File Name	: generic.vert
	Author		: Bora Yalciner
	Description	:

		Basic vertex transform shader...
		Just multiplies each vertex to MVP matrix
		and sends it to the rasterizer

		It supports custom per-vertex normals
		and texture (uv) coordinates.
*/

// Definitions
#define IN_POS			layout(location = 0)
#define IN_NORMAL		layout(location = 1)
#define IN_UV			layout(location = 2)
#define IN_COLOR		layout(location = 3)

#define OUT_UV			layout(location = 0)
#define OUT_NORMAL		layout(location = 1)
#define OUT_WORLD_POS	layout(location = 2) // world position to send to fragment shader for lighting
#define OUT_HEIGHT		layout(location = 3) // height info to send to fragment shader

#define U_TRANSFORM_MODEL	layout(location = 0)
#define U_TRANSFORM_VIEW	layout(location = 1)
#define U_TRANSFORM_PROJ	layout(location = 2)
#define U_TRANSFORM_NORMAL	layout(location = 3)

// Input
in IN_POS	 vec3 vPos;
in IN_NORMAL vec3 vNormal;
in IN_UV	 vec2 vUV;

// Output
// This parameter goes to rasterizer
// This is mandatory since we are using modern pipeline
out gl_PerVertex {vec4 gl_Position;};

// These pass through to rasterizer and will be iterpolated at
// fragment positions
out OUT_UV		vec2 fUV;
out OUT_NORMAL	vec3 fNormal;
out OUT_HEIGHT	float fHeight; // height info to send to fragment shader
out OUT_WORLD_POS vec3 fWorldPos;

// Uniforms
U_TRANSFORM_MODEL	uniform mat4 uModel;
U_TRANSFORM_VIEW	uniform mat4 uView;
U_TRANSFORM_PROJ	uniform mat4 uProjection;
U_TRANSFORM_NORMAL  uniform mat3 uNormalMatrix;

layout(location = 4) uniform float uHeightScale; // height scale for height scaling

void main(void)
{
	fUV = vUV;
	fHeight = vPos.y; // height info to send to fragment shader

	vec3 scaledPos = vec3(vPos.x, vPos.y * uHeightScale, vPos.z);

	fWorldPos = vec3(uModel * vec4(scaledPos, 1.0)); // world position to send to fragment shader for lighting

	// normal correction for height scaling
	vec3 correctedNormal = normalize(vNormal * vec3(1.0, 1.0 / uHeightScale, 1.0));
	fNormal = normalize(uNormalMatrix * correctedNormal);

	// Rasterizer
	gl_Position = uProjection * uView * uModel * vec4(scaledPos.xyz, 1.0f);
}