#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_colour;

void main()
{
	out_colour = vec4(1.0, 1.0, 1.0, 1.0);
}