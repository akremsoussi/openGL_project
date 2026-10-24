//-----------------------------------------------------------------------------
// Fragment shader for the point light shadow maps: instead of the usual depth,
// store the distance to the light (divided by farPlane to fit in [0, 1]).
// The scene shader compares it with its own distance to the light.
//-----------------------------------------------------------------------------
#version 330 core

struct Material
{
    sampler2D diffuseMap;
    int hasDiffuseMap;
};

in vec3 FragPos;
in vec2 TexCoord;

uniform Material material;
uniform vec3 lightPos;
uniform float farPlane;

void main()
{
	// Transparent texels (leaves) cast no shadow
	if (material.hasDiffuseMap == 1 && texture(material.diffuseMap, TexCoord).a < 0.5f)
		discard;

	gl_FragDepth = length(FragPos - lightPos) / farPlane;
}
