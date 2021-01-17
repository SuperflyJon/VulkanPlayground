#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput depthBuffer;

layout (location = 0) out vec4 outColor;

void main() 
{
	float depth = subpassLoad(depthBuffer).r;
	float cutOff = 0.99;
	depth = (depth - cutOff) * (1 / (1 - cutOff));
	outColor = vec4(vec3(depth), 0);
}
