#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput inPosition;
layout (input_attachment_index = 0, binding = 1, set = 0) uniform subpassInput inAlbedo;
layout (input_attachment_index = 0, binding = 2, set = 0) uniform subpassInput inNormal;

layout(push_constant) uniform PushConst {
	int attachmentType;
} pushConst;

layout (location = 0) out vec4 outColour;

void main() 
{
	switch (pushConst.attachmentType)
	{
		case 0:
			discard;	// Ignore test
		break;
		case 1:
			outColour = subpassLoad(inPosition);
		break;
		case 2:
			outColour = subpassLoad(inAlbedo);
		break;
		case 3:
			outColour = subpassLoad(inNormal);
		break;
	}
}
