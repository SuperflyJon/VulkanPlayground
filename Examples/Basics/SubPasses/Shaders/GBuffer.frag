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

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outNormal;

layout(constant_id = 0) const float NEAR_PLANE = 0.0f;
layout(constant_id = 1) const float FAR_PLANE = 0.0f;
float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main() 
{
	// Test lighting for comparison
	vec3 totColour = vec3(0);

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

	// Store g-buffer details
	outPosition = vec4(lightFragPos, 1.0);
	// Store linearized depth in alpha component
	outPosition.a = linearDepth(gl_FragCoord.z);

	outNormal = vec4(lightNormal, 1.0);
	outAlbedo = vec4(fragColour, 0.0);
}
