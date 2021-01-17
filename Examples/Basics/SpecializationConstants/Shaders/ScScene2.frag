#version 450

layout(binding = 3) uniform LightToonUBO
{
	vec4 cutoff;
	vec4 value;
	vec3 lightPos;
} lightInfo;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;

layout(location = 0) out vec4 outColour;

vec3 desaturate(vec3 color, float amount)
{
    vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), color));
    return vec3(mix(color, gray, amount));
}

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
	outColour = vec4(desaturate(fragColour * shade, 4), 1);	// Mess with the colours for fun
}
