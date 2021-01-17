#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;	// Unused but used by other shader

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1);
}
