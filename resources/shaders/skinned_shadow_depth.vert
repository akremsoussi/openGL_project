//-----------------------------------------------------------------------------
// Vertex shader for the shadow map pass of animated characters:
// the skinning of skinned.vert, seen from the light.  Use with shadow_depth.frag
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in uvec4 joints;
layout (location = 4) in vec4 weights;

#define MAX_BONES 256	// must match MAX_BONES in SkinnedModel.h

layout (std140) uniform Bones
{
	mat4 bones[MAX_BONES];
};
uniform mat4 model;					// model matrix
uniform mat4 lightSpaceMatrix;		// light projection * light view

out vec2 TexCoord;

void main()
{
	mat4 skin = weights.x * bones[joints.x]
	          + weights.y * bones[joints.y]
	          + weights.z * bones[joints.z]
	          + weights.w * bones[joints.w];

	TexCoord = texCoord;
	gl_Position = lightSpaceMatrix * model * skin * vec4(pos, 1.0f);
}
