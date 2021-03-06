#version 450

layout (input_attachment_index = 0, binding = 1, set = 0) uniform subpassInput samplerPositionDepth;
layout (binding = 2) uniform sampler2D samplerTexture;

layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (constant_id = 0) const float NEAR_PLANE = 0.1f;
layout (constant_id = 1) const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main () 
{
	// Sample depth from deferred depth buffer and check if visible
	float depth = subpassLoad(samplerPositionDepth).a;
	if ((depth == 0.0) || (linearDepth(gl_FragCoord.z) < depth))
		outColor = texture(samplerTexture, inUV);
	else
		discard;
}
