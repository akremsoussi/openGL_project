//-----------------------------------------------------------------------------
// Vertex shader for the point light shadow maps: animated characters
// (the skinning of skinned.vert).  Use with point_shadow_depth.frag
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
uniform mat4 lightSpaceMatrix;		// projection * view of the cube face

out vec3 FragPos;
out vec2 TexCoord;

void main()
{
	mat4 skin = weights.x * bones[joints.x]
	          + weights.y * bones[joints.y]
	          + weights.z * bones[joints.z]
	          + weights.w * bones[joints.w];

	FragPos = vec3(model * skin * vec4(pos, 1.0f));
	TexCoord = texCoord;
	gl_Position = lightSpaceMatrix * vec4(FragPos, 1.0f);
}
