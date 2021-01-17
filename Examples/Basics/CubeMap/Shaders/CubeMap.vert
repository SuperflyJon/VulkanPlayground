
#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 1) out vec3 fragTexCoord;

void main()
{
	fragTexCoord = inPosition;	// Map from position to cube map

	mat4 modelToWorldNoTranslation = mat4(mat3(ubo.view * ubo.model));	// Ignore translations
	gl_Position = ubo.projection * modelToWorldNoTranslation * vec4(inPosition, 1.0);
	gl_Position = gl_Position.xyww;	// Fix depth == 1 (w / w)
}
