#version 450

layout (binding = 1) uniform sampler2D texSampler;

layout (location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = texture(texSampler, inUV);
	outColour.a = 1.0f;
}
