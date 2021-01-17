
#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 1) uniform TimeUBO
{
	float time;
} rotTime;

layout(binding = 2) uniform DynamicUniformBufferObject
{
	vec3 rotation;
	vec3 offset;
} dynubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 lightPos;
layout(location = 2) out vec3 lightNormal;

void main()
{
	fragTexCoord = inTexCoord;
	// Perform matrix operatoins in the shader...
	vec3 s = sin(dynubo.rotation * rotTime.time);
	vec3 c = cos(dynubo.rotation * rotTime.time);
	mat4 modelToWorld = ubo.model * mat4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, dynubo.offset.x, dynubo.offset.y, dynubo.offset.z, 1.0)
				* mat4(1, 0, 0, 0, 0, c.x, s.x, 0, 0, -s.x, c.x, 0, 0, 0, 0, 1)
				* mat4(c.y, 0, -s.y, 0, 0, 1, 0, 0, s.y, 0, c.y, 0, 0, 0, 0, 1)
				* mat4(c.z, s.z, 0, 0, -s.z, c.z, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	vec4 positionInWorld = modelToWorld * vec4(inPosition, 1.0f);	// Vertex Position in world co-ords
	gl_Position = ubo.projection * ubo.view * positionInWorld;	// Projected vertex Position

	// Light calculations in world space
	lightPos = vec3(positionInWorld);
	lightNormal = normalize(mat3(ubo.model) * inNormal);
}
