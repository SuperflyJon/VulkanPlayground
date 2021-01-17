#version 450

layout(binding = 2) uniform sampler3D texSamplerArray;

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColour;

void main()
{
	float grey = texture(texSamplerArray, fragTexCoord).r;	// Just a single value, so repeat red channel for grey scale output
    outColour = vec4(vec3(grey), 1.0);
}
