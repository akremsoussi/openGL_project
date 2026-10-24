//-----------------------------------------------------------------------------
// Moon / sun: one square facing the camera around "center".  The square is 3x
// the disk radius so that the halo around the disk fits in it.
//-----------------------------------------------------------------------------
#version 330 core

uniform mat4 view;
uniform mat4 projection;
uniform vec3 center;		// world position of the disk
uniform float radius;		// disk radius

out vec2 uv;				// -1..1 on the square, disk edge at 1/3

void main()
{
	vec2 corners[4] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));
	uv = corners[gl_VertexID];

	vec4 c = view * vec4(center, 1.0);
	c.xy += uv * radius * 3.0;
	gl_Position = projection * c;
}
