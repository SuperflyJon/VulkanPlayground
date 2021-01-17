#version 450

layout (constant_id = 0) const int NUM_TEXTURES = 1;

layout(binding = 2) uniform sampler texSampler;
layout(binding = 3) uniform texture2D textures[NUM_TEXTURES];

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int textureNumber;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = texture(sampler2D(textures[textureNumber], texSampler), fragTexCoord);
}
