#version 450

layout(location = 0) out vec4 outFragColour;	// 0 = colour attachment number

void main()
{
	outFragColour = vec4(vec3(0.8), 1.0);
}
