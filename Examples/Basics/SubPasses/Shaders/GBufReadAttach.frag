#version 450

layout (input_attachment_index = 0, binding = 0, set = 0) uniform subpassInput colour;
layout (input_attachment_index = 1, binding = 1, set = 0) uniform subpassInput position;
layout (input_attachment_index = 2, binding = 2, set = 0) uniform subpassInput albedo;
layout (input_attachment_index = 3, binding = 3, set = 0) uniform subpassInput normal;
layout (input_attachment_index = 4, binding = 4, set = 0) uniform subpassInput depth;

layout(location = 0) in vec2 coords;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform PushConst {
	int attachmentType;
} pushConst;

void main() 
{
	int displayType = pushConst.attachmentType;
	if (displayType == 6)
	{
		int column = int(coords.x * 3);
		int row = int(coords.y * 2);
		displayType = row * 3 + column;
	}

	switch (displayType)
	{
		case 0:	// Main colour attachment
			outColor = subpassLoad(colour);
		break;
		case 1:	// Normal info
			outColor = vec4(subpassLoad(normal).rgb, 0);
		break;
		case 2:	// Albedo colour attachment
			outColor = vec4(subpassLoad(albedo).rgb, 0);
		break;
		case 3:	// Main depth attachment
		{
			float depth = subpassLoad(depth).r;
			float cutOff = 0.98;
			depth = (depth - cutOff) * (1 / (1 - cutOff));
			outColor = vec4(vec3(depth), 0);
			break;
		}
		case 4:	// Position info
			outColor = vec4(subpassLoad(position).rgb, 0);
		break;
		case 5:	// Linear depth attachment
			float depth = subpassLoad(position).a;
			outColor = vec4(vec3(depth / 10.0f), 0);
		break;
	}
}
