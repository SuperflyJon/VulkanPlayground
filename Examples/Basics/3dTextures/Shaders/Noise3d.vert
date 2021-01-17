#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 1) uniform UBODepth
{
	float depth;
} data;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 1) out vec3 fragTexCoord;

void main() {
	fragTexCoord = vec3(inTexCoord, data.depth);
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
