//-----------------------------------------------------------------------------
// Particles: one square per instance, always facing the camera (billboard).
// The 4 corners come from gl_VertexID, the center and size from the instance.
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec4 positionSize;		// xyz = center, w = size (per instance)
layout (location = 1) in float opacity;			// per instance

uniform mat4 view;
uniform mat4 projection;

out vec2 uv;
out float alpha;

void main()
{
	vec2 corners[4] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));
	uv = corners[gl_VertexID];

	// Offset the corner in view space: the square faces the camera
	vec4 center = view * vec4(positionSize.xyz, 1.0);
	center.xy += uv * positionSize.w * 0.5;

	gl_Position = projection * center;
	alpha = opacity;
}
