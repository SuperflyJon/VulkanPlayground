
#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 1) uniform DynamicUniformBufferObject
{
	mat4 model;
	int textureNumber;
} dynubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int textureNumber;

void main() {
	fragTexCoord = inTexCoord;
	textureNumber = dynubo.textureNumber;
	gl_Position = ubo.projection * ubo.view * ubo.model * dynubo.model * vec4(inPosition, 1.0);
}
