#version 450

layout (binding = 1) uniform sampler2D offscreenBuffer;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColour;

void main() 
{
	outColour = texture(offscreenBuffer, inUV);
}
