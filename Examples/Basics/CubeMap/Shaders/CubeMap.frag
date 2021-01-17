#version 450

layout(binding = 1) uniform samplerCube cubeSampler;

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = texture(cubeSampler, fragTexCoord);
}
