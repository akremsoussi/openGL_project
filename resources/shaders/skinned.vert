//-----------------------------------------------------------------------------
// Vertex shader for animated characters (skinning)
//
// Same outputs as lighting_dir_point_spot.vert so it works with scene.frag.
// Each vertex is moved by up to 4 bones of the skeleton, mixed by its weights.
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in uvec4 joints;		// bones moving this vertex
layout (location = 4) in vec4 weights;		// how much each bone moves it (sum = 1)

#define MAX_BONES 256	// must match MAX_BONES in SkinnedModel.h

// Pose of each bone, in a uniform buffer (set by SkinnedModel)
layout (std140) uniform Bones
{
	mat4 bones[MAX_BONES];
};
uniform mat4 model;				// model matrix
uniform mat4 view;				// view matrix
uniform mat4 projection;		// projection matrix

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
	// Weighted mix of the bone matrices
	mat4 skin = weights.x * bones[joints.x]
	          + weights.y * bones[joints.y]
	          + weights.z * bones[joints.z]
	          + weights.w * bones[joints.w];

	mat4 skinnedModel = model * skin;

    FragPos = vec3(skinnedModel * vec4(pos, 1.0f));						// vertex position in world space
    Normal = mat3(transpose(inverse(skinnedModel))) * normal;			// normal direction in world space

	TexCoord = texCoord;

	gl_Position = projection * view * vec4(FragPos, 1.0f);
}
