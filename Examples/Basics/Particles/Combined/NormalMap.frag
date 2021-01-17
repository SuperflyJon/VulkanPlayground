#version 450

layout(binding = 2) uniform LightUBO
{
	vec3 lightPos;	// These positions not used as they are passed in, in correct space
	vec3 viewPos;

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
} lightInfo;

layout (binding = 3) uniform sampler2D texSampler;
layout (binding = 4) uniform sampler2D normalMapSampler;

in VtoFBlock
{
layout(location = 1) vec3 lightSpace_FragPos;
layout(location = 2) vec3 lightSpace_LightPos;
layout(location = 3) vec3 lightSpace_ViewPos;
layout(location = 4) vec2 UV;
} In;

layout(constant_id = 0) const int showNormals = 0;

layout (location = 0) out vec4 outColour;

float CalcAttenuation()
{
	float distance = length(In.lightSpace_LightPos - In.lightSpace_FragPos);
	float constant = 1.0f;
	float linear =  0.0006f;
	float quadratic = 0.00025f;
	return 1.0f / (constant + linear * distance + quadratic * (distance * distance));
}

void main() 
{
	vec3 totColour = vec3(0);
	vec3 fragColour = vec3(texture(texSampler, In.UV));
	totColour += fragColour * lightInfo.ambientLevel;
	
	vec3 N = texture(normalMapSampler, In.UV).rgb;
	// Transform normal vector to range [-1,1]
	N = normalize(N * 2.0 - 1.0);

	vec3 L = normalize(In.lightSpace_LightPos - In.lightSpace_FragPos);
	float diffusePoint = max(dot(N, L), 0.0f);
	totColour += diffusePoint * fragColour * lightInfo.diffuseLevel;

	vec3 VtoFrag = normalize(In.lightSpace_ViewPos - In.lightSpace_FragPos);
	vec3 R = reflect(-L, N);
	float phongTerm = max(dot(R, VtoFrag), 0.0);
	float specularPoint = pow(phongTerm, lightInfo.shineness);
	totColour += specularPoint * lightInfo.specularLevel;

	outColour = vec4(totColour * CalcAttenuation(), 1);
}
