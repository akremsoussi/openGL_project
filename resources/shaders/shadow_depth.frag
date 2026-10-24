//-----------------------------------------------------------------------------
// Fragment shader for the shadow map pass.
// Only the depth is written; transparent texels (leaves) cast no shadow.
//-----------------------------------------------------------------------------
#version 330 core

struct Material
{
    sampler2D diffuseMap;
    int hasDiffuseMap;
};

in vec2 TexCoord;

uniform Material material;

void main()
{
	if (material.hasDiffuseMap == 1 && texture(material.diffuseMap, TexCoord).a < 0.5f)
		discard;
}
