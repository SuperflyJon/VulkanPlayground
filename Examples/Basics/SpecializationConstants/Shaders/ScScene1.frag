#version 450

layout(binding = 1) uniform LightUBO
{
	vec3 lightPos;
	vec3 viewPos;	// Not used as eye light space has this as the origin

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
} lightInfo;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;

layout(location = 0) out vec4 outColour;

void main()
{
	vec3 totColour = vec3(0);

	totColour += fragColour * lightInfo.ambientLevel;

	vec3 N = normalize(lightNormal);
	vec3 L = normalize(lightInfo.lightPos - lightFragPos);
	float diffusePoint = max(dot(N, L), 0.0f);
	totColour += diffusePoint * fragColour * lightInfo.diffuseLevel;

	vec3 V = normalize(-lightFragPos);
	vec3 R = reflect(-L, N);
	float phongTerm = max(dot(R, V), 0.0);
	float specularPoint = pow(phongTerm, lightInfo.shineness);
	totColour += specularPoint * lightInfo.specularLevel;

	outColour = vec4(totColour, 1);
}
