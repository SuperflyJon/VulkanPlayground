#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput inPosition;
layout (input_attachment_index = 0, binding = 1, set = 0) uniform subpassInput inAlbedo;
layout (input_attachment_index = 0, binding = 2, set = 0) uniform subpassInput inNormal;

layout (location = 0) out vec4 outColour;

layout(binding = 3) uniform LightUBO
{
	vec3 lightPos;
	vec3 viewPos;

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
} lightInfo;

void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 lightFragPos = subpassLoad(inPosition).rgb;
	vec3 lightNormal = subpassLoad(inNormal).rgb;
	vec3 fragColour = subpassLoad(inAlbedo).rgb;

	vec3 totColour = vec3(0);

	vec3 L = normalize(lightInfo.lightPos - lightFragPos);
	float diffusePoint = max(dot(lightNormal, L), 0.0f);

	totColour += fragColour * lightInfo.ambientLevel;
	totColour += diffusePoint * fragColour * lightInfo.diffuseLevel;

	vec3 VtoFrag = normalize(lightInfo.viewPos - lightFragPos);
	vec3 R = reflect(-L, lightNormal);
	float phongTerm = max(dot(R, VtoFrag), 0.0);
	float specularPoint = pow(phongTerm, lightInfo.shineness);
	totColour += specularPoint * lightInfo.specularLevel;

	outColour = vec4(min(totColour, vec3(1.0f)), 0);
}
