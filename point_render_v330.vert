#version 330

uniform mat4 MV;
uniform mat4 P;

layout (location = 0) in vec4 vertex;

void main()
{
	gl_Position = P * MV * vertex;
}
