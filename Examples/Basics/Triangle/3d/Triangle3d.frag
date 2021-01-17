#version 450

layout(location = 0) in vec3 fragColour;

layout(location = 0) out vec4 outFragColour;	// 0 = colour attachment number

void main()
{
	outFragColour = vec4(fragColour, 1.0);
}
