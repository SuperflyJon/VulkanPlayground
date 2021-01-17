#version 450

layout (binding = 2) uniform sampler2D samplerTexture;

layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main () 
{
	outColor = texture(samplerTexture, inUV);
}
