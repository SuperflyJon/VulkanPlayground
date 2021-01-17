#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 1) out vec2 outUV;

void main()
{
	outUV = inUV;
	vec4 positionInWorld = ubo.model * vec4(inPosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * positionInWorld;
}
