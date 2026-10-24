//-----------------------------------------------------------------------------
// Bloom step 3: scene + blurred bright parts, then tone mapping.
// Ordinary colors (below 0.8) are kept as they are; only the highlights are
// compressed smoothly toward 1 instead of being cut.
//-----------------------------------------------------------------------------
#version 330 core

in vec2 uv;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform int bloomEnabled;

out vec4 color;

void main()
{
	vec3 hdr = texture(scene, uv).rgb;
	if (bloomEnabled != 0)
		hdr += texture(bloom, uv).rgb * 0.5;

	hdr = max(hdr, vec3(0.0));
	vec3 base = min(hdr, vec3(0.8));
	vec3 highlights = max(hdr - vec3(0.8), vec3(0.0));
	vec3 mapped = base + 0.2 * (vec3(1.0) - exp(-highlights / 0.2));

	color = vec4(mapped, 1.0);
}
