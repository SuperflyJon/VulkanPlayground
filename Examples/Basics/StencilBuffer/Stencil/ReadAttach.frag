#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform usubpassInput depthBuffer;

layout (location = 0) out vec4 outColor;

void main() 
{
	float stencil = subpassLoad(depthBuffer).r;
	outColor = vec4(vec3(stencil / 255), 0);
}
