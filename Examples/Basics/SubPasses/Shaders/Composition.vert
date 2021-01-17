#version 450

void main() 
{
	const vec2 fullscreenQuad[6] = { vec2(-1,1), vec2(1,1), vec2(1,-1), vec2(-1,-1), vec2(-1,1), vec2(1,-1) };
	gl_Position = vec4(fullscreenQuad[gl_VertexIndex], 0.0f, 1.0f);
}
