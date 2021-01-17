#version 450

layout (binding = 1) uniform sampler2D samplerFlame;
layout (binding = 2) uniform sampler2D samplerSmoke;

layout (location = 0) in float inAlpha;
layout (location = 1) in float inRotation;
layout (location = 2) flat in int inType;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConsts
{
	int drawType;
} info;

void main () 
{
	// Rotate texture coordinates
	float rotCenter = 0.5;
	float rotCos = cos(inRotation);
	float rotSin = sin(inRotation);
	vec2 rotUV = vec2(
		rotCos * (gl_PointCoord.x - rotCenter) + rotSin * (gl_PointCoord.y - rotCenter) + rotCenter,
		rotCos * (gl_PointCoord.y - rotCenter) - rotSin * (gl_PointCoord.x - rotCenter) + rotCenter);

	// Flame
	if (info.drawType != inType)
		discard;

	if (inType == 0)
	{
		outFragColor = texture(samplerFlame, rotUV) * inAlpha;
		outFragColor.a = 1.0f;	// Blend everything
	}
	else
	{
		vec4 texSample = texture(samplerSmoke, rotUV);
		if (texSample.a < 0.1f)
			discard;	// Clip the edges

		outFragColor = texSample * inAlpha;
		outFragColor.a = 0.0f;	// Don't add any more on top
	}
}
