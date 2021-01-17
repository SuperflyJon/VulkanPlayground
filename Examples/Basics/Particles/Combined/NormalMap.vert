#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 1) uniform VertInfo
{
	vec3 lightPos;
	vec3 viewPos;
} vertInfo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

out VtoFBlock
{
layout(location = 1) vec3 lightSpace_FragPos;
layout(location = 2) vec3 lightSpace_LightPos;
layout(location = 3) vec3 lightSpace_ViewPos;
layout(location = 4) vec2 UV;
} Out;

mat3 CalcTBN_WorldToTangent(mat3 model)
{
	vec3 T = normalize(model * inTangent);
	vec3 B = normalize(model * inBiTangent);
	vec3 N = normalize(model * inNormal);
	return transpose(mat3(T, B, N));
}

void main()
{
	Out.UV = inUV;

	vec4 positionInWorld = ubo.model * vec4(inPosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * positionInWorld;

	// Light calculations in tangent space
	mat4 worldToLight = mat4(CalcTBN_WorldToTangent(mat3(ubo.model)));	// Can use transpose(inverse(ubo.model)) if non-uniform scaling in scene

	Out.lightSpace_FragPos = vec3(worldToLight * positionInWorld);	// Vertex Position in light co-ords
	Out.lightSpace_LightPos = vec3(worldToLight * vec4(vertInfo.lightPos, 1.0f));
	Out.lightSpace_ViewPos = vec3(worldToLight * vec4(vertInfo.viewPos, 1.0f));
}
