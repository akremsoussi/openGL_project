//-----------------------------------------------------------------------------
// Vertex shader for the shadow map pass (depth seen from the light)
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;					// model matrix
uniform mat4 lightSpaceMatrix;		// light projection * light view

out vec2 TexCoord;

void main()
{
	TexCoord = texCoord;
	gl_Position = lightSpaceMatrix * model * vec4(pos, 1.0f);
}
