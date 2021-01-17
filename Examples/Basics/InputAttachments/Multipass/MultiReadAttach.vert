#version 450

layout(location = 0) out vec2 coords;

void main() 
{
	const vec2 vertices[6] = { vec2(-1,1), vec2(1,1), vec2(1,-1), vec2(-1,-1), vec2(-1,1), vec2(1,-1) };
	gl_Position = vec4(vertices[gl_VertexIndex], 0.0f, 1.0f);
	coords = vertices[gl_VertexIndex] + 1;
}
