#version 450

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = texture(texSampler, fragTexCoord);
}
