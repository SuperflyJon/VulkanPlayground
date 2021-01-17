#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in float inAlpha;
layout (location = 2) in float inSize;
layout (location = 3) in float inRotation;
layout (location = 4) in int inType;

layout (location = 0) out float outAlpha;
layout (location = 1) out float outRotation;
layout (location = 2) out int outType;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(binding = 3) uniform UBO
{
	float viewPortX;
} misc;

void main () 
{
	outAlpha = inAlpha;
	outRotation = inRotation;
	outType = inType;

	vec4 Pos = vec4(inPos.xyz, 1.0);
	gl_Position = ubo.projection * ubo.view * ubo.model * Pos;	
	
	// Base size of the point sprites
	float spriteSize = 8.0 * inSize;

	// Scale particle size depending on camera projection
	vec4 eyePos = ubo.view * ubo.model * Pos;
	vec4 projectedCorner = ubo.projection * vec4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w);
	gl_PointSize = misc.viewPortX * projectedCorner.x / projectedCorner.w;	
}
