#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D samplerOffscreen;

layout (location = 0) in vec2 inTextureCoords;
layout (location = 1) in vec4 inPosition;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConsts
{
	int showBlur;
} test;

void main() 
{
	outFragColor = texture(samplerColor, inTextureCoords) * 0.5;

	vec4 projCoord = inPosition * vec4(1.0 / inPosition.w);

	projCoord += vec4(1.0);
	projCoord *= vec4(0.5);

	if (test.showBlur == 0)
	{
		outFragColor += texture(samplerOffscreen, vec2(projCoord.s, projCoord.t)) * 0.5;
	}
	else
	{
		// Slow single pass blur for demonstration purposes only
		const float blurSize = 1.0 / 512.0;	

		vec4 reflection = vec4(0.0);
		for (int x = -3; x <= 3; x++)
		{
			for (int y = -3; y <= 3; y++)
			{
				reflection += texture(samplerOffscreen, vec2(projCoord.s + x * blurSize, projCoord.t + y * blurSize)) / 49.0;
			}
		}
		outFragColor += reflection * 0.5;
	}
}
