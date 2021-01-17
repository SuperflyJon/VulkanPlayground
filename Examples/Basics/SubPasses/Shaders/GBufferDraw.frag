#version 450

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 lightPos;
layout(location = 2) in vec3 lightNormal;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outNormal;

void main() 
{
	outPosition = vec4(lightPos, 1.0);
	outNormal = vec4(lightNormal, 1.0);
	outAlbedo = vec4(fragColour, 1.0);
}
