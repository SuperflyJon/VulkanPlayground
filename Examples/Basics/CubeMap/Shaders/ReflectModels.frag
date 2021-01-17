#version 450

layout(binding = 1) uniform samplerCube cubeSampler;

layout(binding = 2) uniform LightUBO
{
	vec3 lightPos;
	vec3 viewPos;

	float ambientLevel;
	float diffuseLevel;
	float specularLevel;
	float shineness;
} lightInfo;

layout(location = 1) in vec3 lightFragPos;
layout(location = 2) in vec3 lightNormal;
layout(location = 3) in vec3 reflectPos;
layout(location = 4) in vec3 reflectNormal;

layout(push_constant) uniform ReflectPushConsts {
	int showReflection;
} ReflectInfo;

layout(location = 0) out vec4 outColour;

vec3 desaturate(vec3 color, float amount)
{
    vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), color));
    return vec3(mix(color, gray, amount));
}

void main()
{
	vec3 reflectI = normalize(reflectPos);// - lightInfo.viewPos);
	vec3 reflectR = reflect(reflectI, normalize(reflectNormal));
//	reflectR.y = -reflectR.y;	// Texture already smapled upside down...
	reflectR.x = -reflectR.x;	// Flip otherwise back to front

	vec3 reflectColour = texture(cubeSampler, reflectR).rgb;
	if (ReflectInfo.showReflection == 0)
		reflectColour = vec3(1);
	else
		reflectColour *= 1.2f;

	vec3 totColour = vec3(0);

	vec3 fragColour = desaturate(reflectColour, 0.65);
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
