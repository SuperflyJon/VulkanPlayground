
#version 450

layout (constant_id = 0) const int NUM_TEXTURES = 1;
layout (constant_id = 1) const int NUM_OBJECTS = 1;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 1) uniform ObjectsUniformBufferObject
{
	mat4 model[NUM_OBJECTS];
} objects;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 1) out vec3 fragTexCoord;

void main()
{
	float increment = float(NUM_TEXTURES) / NUM_OBJECTS;
	fragTexCoord = vec3(inTexCoord, gl_InstanceIndex * increment - 0.2f);
	gl_Position = ubo.projection * ubo.view * ubo.model * objects.model[gl_InstanceIndex] * vec4(inPosition, 1.0);
}
