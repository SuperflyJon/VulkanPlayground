#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 1) out vec2 outUV;

void main () 
{
	outUV = inUV;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0f);
}