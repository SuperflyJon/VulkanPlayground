#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 1) out vec3 lightPos;
layout(location = 2) out vec3 lightNormal;
layout(location = 3) out vec3 reflectPos;
layout(location = 4) out vec3 reflectNormal;

void main()
{
	vec4 positionInWorld = ubo.model * vec4(inPosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * positionInWorld;

	// Reflection calculation vectors
	reflectPos = vec3(positionInWorld);
	reflectNormal = mat3(ubo.model) * inNormal;

	// Light calculations in world space
	lightPos = vec3(positionInWorld);
	lightNormal = normalize(mat3(ubo.model) * inNormal);
}
