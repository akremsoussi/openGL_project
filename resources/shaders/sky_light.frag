//-----------------------------------------------------------------------------
// Moon / sun: bright disk (a little darker on its edge) and a soft halo
//-----------------------------------------------------------------------------
#version 330 core

in vec2 uv;

uniform vec3 color;		// can be brighter than 1 (bloom)

out vec4 frag_color;

void main()
{
	float r = length(uv) * 3.0;		// 1 = edge of the disk

	float disk = 1.0 - smoothstep(0.95, 1.0, r);
	float limb = 0.8 + 0.2 * sqrt(max(1.0 - r * r, 0.0));	// edge darkening
	float halo = exp(-2.5 * max(r - 1.0, 0.0)) * (1.0 - disk) * 0.25;

	if (disk + halo < 0.002)
		discard;

	// Added to the sky (additive blending)
	frag_color = vec4(color * (disk * limb + halo), 1.0);
}
