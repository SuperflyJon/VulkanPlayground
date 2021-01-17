#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inSize;
layout (location = 2) in uvec2 inTextorg;
layout (location = 3) in uvec2 inTextoff;
layout (location = 4) in vec3 inColour;
layout (location = 5) in int inColoured;

layout (location = 0) out vec2 outPosition;
layout (location = 1) out vec2 outSize;
layout (location = 2) out uvec2 outTextorg;
layout (location = 3) out uvec2 outTextoff;
layout (location = 4) out vec3 outColour;
layout (location = 5) out int outColoured;

void main()
{
	outPosition = inPosition;
	outSize = inSize;
	outTextorg = inTextorg;
	outTextoff = inTextoff;
	outColour = inColour;
	outColoured = inColoured;
}
