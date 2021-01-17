#version 450

layout (location = 0) out vec2 outUV;

void main() 
{
	const vec2 fullscreenQuad[6] = { vec2(-1,1), vec2(1,1), vec2(1,-1), vec2(-1,-1), vec2(-1,1), vec2(1,-1) };
	gl_Position = vec4(fullscreenQuad[gl_VertexIndex], 0.0f, 1.0f);

	const vec2 quadTextureCoords[6] = { vec2(0,1), vec2(1,1), vec2(1,0), vec2(0,0), vec2(0,1), vec2(1,0) };
	outUV = vec2(quadTextureCoords[gl_VertexIndex]);
}
