#version 450

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightPos;
layout(location = 2) in vec3 lightNormal;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outNormal;

layout (constant_id = 0) const float NEAR_PLANE = 0.0f;
layout (constant_id = 1) const float FAR_PLANE = 0.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main() 
{
	outPosition = vec4(lightPos, 1.0);
	// Store linearized depth in alpha component
	outPosition.a = linearDepth(gl_FragCoord.z);

	outNormal = vec4(lightNormal, 1.0);
	outAlbedo = vec4(fragColour, 0.0);
}
