#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main() 
{
	// Extrude along normal
	vec4 pos = vec4(inPosition.xyz + inNormal * 0.1f, 1.0f);
	gl_Position = ubo.projection * ubo.view * ubo.model * pos;
}
