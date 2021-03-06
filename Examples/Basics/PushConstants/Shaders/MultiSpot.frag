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

layout(binding = 3) uniform SpotUBO
{
	int hardSpot;
	int showSpot;
	float diffuseLevel;
} spotSettings;

#define NUM_SPOTS 6

layout(push_constant) uniform SpotPushConsts {
	vec4 lookatPos[NUM_SPOTS];
} spotInfo;

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

	vec3 VtoFrag = normalize(lightInfo.viewPos - lightFragPos);
	vec3 R = reflect(-L, N);
	float phongTerm = max(dot(R, VtoFrag), 0.0);
	float specularPoint = pow(phongTerm, lightInfo.shineness);
	totColour += specularPoint * lightInfo.specularLevel;

	bool anySpots = false;
	for (int spotNum = 0; spotNum < NUM_SPOTS; spotNum++)
	{
		vec3 spotDir = normalize(vec3(lightInfo.lightPos) - spotInfo.lookatPos[spotNum].xyz);

		if (spotSettings.showSpot == 1 && dot(spotDir,L) > spotInfo.lookatPos[spotNum].w)
		{	// inside the cone
			if (spotSettings.hardSpot == 1 && !anySpots)
			{
				totColour = vec3(0);	// Reset world light
				anySpots = true;
			}
			vec3 spotColours[NUM_SPOTS]= { vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 0.0)};
			vec3 spotDiffuse = diffusePoint * spotColours[spotNum] * spotSettings.diffuseLevel;
			totColour += spotDiffuse;
		}
	}
	outColour = vec4(min(totColour, vec3(1.0f)), 1);
}
