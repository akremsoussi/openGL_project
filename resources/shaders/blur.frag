//-----------------------------------------------------------------------------
// Bloom step 2: Gaussian blur along one axis (9 samples).  Blurring
// horizontally then vertically gives the same result as a 2D blur, cheaper.
//-----------------------------------------------------------------------------
#version 330 core

in vec2 uv;

uniform sampler2D source;
uniform vec2 axis;		// (1, 0) horizontal or (0, 1) vertical

out vec4 color;

void main()
{
	vec2 stepUV = axis / vec2(textureSize(source, 0));
	float weights[4] = float[](0.194595, 0.121622, 0.054054, 0.016216);

	vec3 sum = texture(source, uv).rgb * 0.227027;
	for (int i = 1; i <= 4; i++)
		sum += (texture(source, uv + stepUV * i).rgb + texture(source, uv - stepUV * i).rgb) * weights[i - 1];

	color = vec4(sum, 1.0);
}
