#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 0) uniform Transform_Ubo
{
	mat4 mvp[256];
} transform_ubo;

layout(location = 0) in vec3 position;

void main()
{
	gl_Position = transform_ubo.mvp[0] * vec4(position, 1.0);
}