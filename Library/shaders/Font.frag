#version 450

layout (binding = 2) uniform sampler2D texSampler;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 fontColour;
layout (location = 2) flat in int coloured;

layout (location = 0) out vec4 outColour;

void main() 
{
	if (coloured == 1)
		outColour = texture(texSampler, inUV);
	else
	{
		float intensity = texture(texSampler, inUV).r;
		outColour = vec4(fontColour, intensity);
	}
}
