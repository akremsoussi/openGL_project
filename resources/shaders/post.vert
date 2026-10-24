//-----------------------------------------------------------------------------
// Full screen triangle without vertex buffer: the 3 corners are computed from
// gl_VertexID ((0,0), (2,0), (0,2) in texture coordinates).
//-----------------------------------------------------------------------------
#version 330 core

out vec2 uv;

void main()
{
	vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
	uv = p;
	gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
