#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1, set = 0) uniform subpassInput inputDepth;

layout(location = 0) in vec2 coords;

layout (location = 0) out vec4 outColor;

void main() 
{
	if (coords.y / coords.x < 1)
	{
		float depth = subpassLoad(inputDepth).r;
		float cutOff = 0.99;
		depth = (depth - cutOff) * (1 / (1 - cutOff));
		outColor = vec4(vec3(depth), 0);
	}
	else
	{
		outColor = subpassLoad(inputColor);
	}
}
