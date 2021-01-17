#version 450

layout(binding = 2) uniform sampler2DArray texSamplerArray;

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColour;

layout(push_constant) uniform PushConsts {
	int multiSample;
} flags;

layout (constant_id = 0) const int NUM_TEXTURES = 1;

void main()
{
	if (flags.multiSample == 1)
	{
		int baseImage = int(fragTexCoord.z);
		int nextImage = min(baseImage + 1, NUM_TEXTURES);
		outColour = texture(texSamplerArray, vec3(fragTexCoord.xy, baseImage)) * (nextImage - fragTexCoord.z) + texture(texSamplerArray, vec3(fragTexCoord.xy, nextImage)) * (fragTexCoord.z - baseImage);
	}
	else
		outColour = texture(texSamplerArray, fragTexCoord);
}
