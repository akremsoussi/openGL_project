//-----------------------------------------------------------------------------
// Vertex shader for the point light shadow maps (cube map faces): OBJ models
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;					// model matrix
uniform mat4 lightSpaceMatrix;		// projection * view of the cube face

out vec3 FragPos;					// world position (for the distance to the light)
out vec2 TexCoord;

void main()
{
	FragPos = vec3(model * vec4(pos, 1.0f));
	TexCoord = texCoord;
	gl_Position = lightSpaceMatrix * vec4(FragPos, 1.0f);
}
