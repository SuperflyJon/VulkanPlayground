#version 450

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(binding = 0) uniform ProjUBO
{
	mat4 projection;
	int texWidth, texHeight;
} ubo;

layout (location = 0) in vec2 inPosition[];
layout (location = 1) in vec2 inSize[];
layout (location = 2) in uvec2 inTextorg[];
layout (location = 3) in uvec2 inTextoff[];
layout (location = 4) in vec3 inColour[];
layout (location = 5) in int inColoured[];

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outFontColour;
layout(location = 2) out int outColoured;

void main(void)
{
	vec2 positions[] = { {0.0f,0.0f}, {0.0f,1.0f}, {1.0f,0.0f}, {1.0f,1.0f} };

	for (int vertex = 0; vertex < 4; vertex++)
	{
		outFontColour = inColour[0];
		outColoured = inColoured[0];
		vec2 vertPos = positions[vertex];
		vec2 pos = inPosition[0] + vertPos * inSize[0];
		outTexCoord = (inTextorg[0] + vertPos * inTextoff[0]) / vec2(ubo.texWidth, ubo.texHeight);
		gl_Position = ubo.projection * vec4(pos, 0.0, 1.0);
		EmitVertex();
	}
}
