#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColour;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColour;
layout(location = 1) out vec3 lightPos;
layout(location = 2) out vec3 lightNormal;
layout(location = 3) out vec2 outUV;

void main()
{
	fragColour = inColour;
	outUV = inUV;

	vec4 positionInWorld = ubo.model * vec4(inPosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * positionInWorld;

	// Light calculations in view space
	lightPos = vec3(ubo.view * positionInWorld);
	lightNormal = normalize(mat3(ubo.view * ubo.model) * inNormal);
}
