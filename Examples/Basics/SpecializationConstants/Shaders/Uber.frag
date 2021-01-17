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

layout(binding = 3) uniform LightToonUBO
{
	vec4 cutoff;
	vec4 value;
	vec3 lightPos;
} lightInfoToon;

layout (constant_id = 0) const int LIGHTING_MODEL = 0;
layout (constant_id = 1) const float PARAM_TOON_DESATURATION = 0.0f;

layout (binding = 2) uniform sampler2D colourmap;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;
layout (location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColour;

vec3 desaturate(vec3 color, float amount)
{
    vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), color));
    return vec3(mix(color, gray, amount));
}

void main()
{
	vec3 totColour = vec3(0);

	switch (LIGHTING_MODEL)
	{
	case 1:
	{
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
		break;
	}
	case 2:
	{
		vec3 N = normalize(lightNormal);
		vec3 L = normalize(lightInfoToon.lightPos - lightFragPos);
		float intensity = dot(N, L);

		float shade = 1.0;
		for (int bracket = 0; bracket < 4; bracket++)
		{
			if (intensity < lightInfoToon.cutoff[bracket])
			{
				shade = lightInfoToon.value[bracket];
				break;
			}
		}
		totColour = desaturate(fragColour * shade, PARAM_TOON_DESATURATION);	// Mess with the colours for fun
		break;
	}
	case 3:
	{
		vec3 texColour = fragColour * texture(colourmap, inUV).r;

		totColour += texColour * (lightInfo.ambientLevel + 0.2f);	// Add 0.2 to brighten scene

		vec3 N = normalize(lightNormal);
		vec3 L = normalize(lightInfo.lightPos - lightFragPos);
		float diffusePoint = max(dot(N, L), 0.0f);
		totColour += diffusePoint * texColour * lightInfo.diffuseLevel;

		vec3 V = normalize(-lightFragPos);
		vec3 R = reflect(-L, N);
		float phongTerm = max(dot(R, V), 0.0);
		float specularPoint = pow(phongTerm, lightInfo.shineness);
		totColour += specularPoint * lightInfo.specularLevel;
		break;
	}
	}
	outColour = vec4(totColour, 1);
}
