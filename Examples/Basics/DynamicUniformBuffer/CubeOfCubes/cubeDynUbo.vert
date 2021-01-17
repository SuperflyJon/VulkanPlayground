
#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
} ubo;

layout(binding = 1) uniform DynamicUniformBufferObject {
	mat4 model;
} dynubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;

void main() {
	fragTexCoord = inTexCoord;
	gl_Position = ubo.proj * ubo.view * dynubo.model * vec4(inPosition, 1.0);
}
