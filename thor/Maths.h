#pragma once

#include "Core.h"



struct Vec_3f
{
	float x, y, z;
};

struct Matrix_4x4
{
	// m11 m12 m13 m14
	// m21 m22 m23 m24
	// m31 m32 m33 m34
	// m41 m42 m43 m44

	// column major
	float32 m11, m21, m31, m41,
		m12, m22, m32, m42,
		m13, m23, m33, m43,
		m14, m24, m34, m44;
};


Vec_3f vec_3f(float32 x, float32 y, float32 z);
Vec_3f vec_3f_add(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_sub(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_mul(Vec_3f v, float32 f);
Vec_3f vec_3f_normalised(Vec_3f v);
float vec_3f_dot(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_cross(Vec_3f a, Vec_3f b);

void matrix_4x4_projection(Matrix_4x4* matrix, float32 fov_y, float32 aspect_ratio, float32 near_plane, float32 far_plane);
void matrix_4x4_translation(Matrix_4x4* matrix, Vec_3f translation);
void matrix_4x4_mul(Matrix_4x4* result, Matrix_4x4* a, Matrix_4x4* b);
void matrix_4x4_lookat(Matrix_4x4* matrix, Vec_3f position, Vec_3f target, Vec_3f up);