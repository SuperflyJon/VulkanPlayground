#version 450

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = vec4(vec3(1.0, 0.5, 0.9), 1.0); 
}
