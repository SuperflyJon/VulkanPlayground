#version 450

layout(location = 0) out vec2 coords;

void main() 
{
	const vec2 fullscreenQuad[6] = { vec2(-1,1), vec2(1,1), vec2(1,-1), vec2(-1,-1), vec2(-1,1), vec2(1,-1) };
	gl_Position = vec4(fullscreenQuad[gl_VertexIndex], 0.0f, 1.0f);

	coords = (fullscreenQuad[gl_VertexIndex] + 1) / 2;	// Split screen into 6 parts
}
