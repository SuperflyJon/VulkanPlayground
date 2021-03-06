#version 450

layout(binding = 2) uniform LightUBO
{
	vec3 lightPos;
	vec3 viewPos;

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
} lightInfo;

layout(push_constant) uniform LoDPushConsts {
	float bias;
} LoDInfo;

layout (binding = 1) uniform sampler2D texSampler;

layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;
layout (location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColour;

void main()
{
	vec3 totColour = vec3(0);
	vec3 fragColour = vec3(texture(texSampler, inUV, LoDInfo.bias));
	totColour += fragColour * lightInfo.ambientLevel;

	vec3 N = normalize(lightNormal);
	vec3 L = normalize(lightInfo.lightPos - lightFragPos);
	float diffusePoint = max(dot(N, L), 0.0f);
	totColour += diffusePoint * fragColour * lightInfo.diffuseLevel;

	vec3 VtoFrag = normalize(lightInfo.viewPos - lightFragPos);
	vec3 R = reflect(-L, N);
	float phongTerm = max(dot(R, VtoFrag), 0.0);
	float specularPoint = pow(phongTerm, lightInfo.shineness);
	totColour += specularPoint * lightInfo.specularLevel;

	outColour = vec4(totColour, 1);
}
