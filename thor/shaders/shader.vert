#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 0) uniform Transform_Ubo
{
	mat4 mvp[8000];
} transform_ubo;

layout(push_constant) uniform Push_Constants
{
  int matrix_i;
} push_constants;

layout(location = 0) in vec3 position;

void main()
{
	gl_Position = transform_ubo.mvp[push_constants.matrix_i] * vec4(position, 1.0);
}