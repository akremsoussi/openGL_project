//-----------------------------------------------------------------------------
// Bloom step 1: keep only the part of each color brighter than white
//-----------------------------------------------------------------------------
#version 330 core

in vec2 uv;

uniform sampler2D source;	// HDR scene

out vec4 color;

void main()
{
	vec3 c = texture(source, uv).rgb;
	float peak = max(max(c.r, c.g), c.b);

	// Same hue, intensity = how much it goes above 1
	color = vec4(c * max(peak - 1.0, 0.0) / max(peak, 0.0001), 1.0);
}
