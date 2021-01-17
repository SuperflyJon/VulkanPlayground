#version 450

layout(binding = 2) uniform sampler2DArray texSamplerArray;

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = texture(texSamplerArray, fragTexCoord);
}
