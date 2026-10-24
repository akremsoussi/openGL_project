//-----------------------------------------------------------------------------
// Particles: soft round snowflake
//-----------------------------------------------------------------------------
#version 330 core

in vec2 uv;
in float alpha;

out vec4 frag_color;

void main()
{
	float radius = length(uv);
	if (radius >= 1.0)
		discard;

	// Opaque center, soft edge
	float glow = 1.0 - smoothstep(0.3, 1.0, radius);
	frag_color = vec4(0.95, 0.97, 1.0, alpha * glow * 0.9);
}
