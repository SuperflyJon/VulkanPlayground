#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTextureCoords;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) out vec2 outTextureCoords;
layout (location = 1) out vec4 outPosition;

void main() 
{
	outTextureCoords = inTextureCoords;
	vec4 positionInWorld = ubo.model * vec4(inPosition, 1.0f);	// Vertex Position in world co-ords
	gl_Position = ubo.projection * ubo.view * positionInWorld;	// Projected vertex Position
	outPosition = gl_Position;
}
