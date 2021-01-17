#version 450

layout(binding = 2) uniform LightToonUBO
{
	vec4 cutoff;
	vec4 value;
	vec3 lightPos;
} lightInfo;
layout(location = 0) in vec3 lightFragPos;
layout(location = 1) in vec3 lightNormal;

layout(location = 0) out vec4 outColour;

void main()
{
	vec3 N = normalize(lightNormal);
	vec3 L = normalize(lightInfo.lightPos - lightFragPos);

	float intensity = dot(N, L);

	float shade = 1.0;
	for (int bracket = 0; bracket < 4; bracket++)
	{
		if (intensity < lightInfo.cutoff[bracket])
		{
			shade = lightInfo.value[bracket];
			break;
		}
	}

	float blue = mix(shade, dot(0.0722, shade), 0.1);	// Desaturate a bit
	outColour = vec4(0.0f, 0.0f, blue, 1.0);
}
