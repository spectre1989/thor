#include "Maths.h"

#include <cmath>



Vec_3f vec_3f(float32 x, float32 y, float32 z)
{
	Vec_3f v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

Vec_3f vec_3f_add(Vec_3f a, Vec_3f b)
{
	return vec_3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec_3f vec_3f_sub(Vec_3f a, Vec_3f b)
{
	return vec_3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec_3f vec_3f_mul(Vec_3f v, float32 f)
{
	return vec_3f(v.x * f, v.y * f, v.z * f);
}

Vec_3f vec_3f_normalised(Vec_3f v)
{
	float32 length_sq = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
	if (length_sq > 0.0f)
	{
		float32 inv_length = 1 / (float32)sqrt(length_sq);
		return vec_3f(v.x * inv_length, v.y * inv_length, v.z * inv_length);
	}

	return v;
}

float vec_3f_dot(Vec_3f a, Vec_3f b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

Vec_3f vec_3f_cross(Vec_3f a, Vec_3f b)
{
	return vec_3f((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
}

Vec_3f vec_3f_lerp(Vec_3f a, Vec_3f b, float32 t)
{
	Vec_3f delta = vec_3f_sub(b, a);
	return vec_3f_add(a, vec_3f_mul(delta, t));
}


void matrix_4x4_projection(
	Matrix_4x4* matrix,
	float32 fov_y, 
	float32 aspect_ratio,
	float32 near_plane, 
	float32 far_plane)
{
	// create projection matrix
	// Note: Vulkan NDC coordinates are top-left corner (-1, -1), z 0-1
	// 1/(tan(fovx/2)*aspect)	0	0				0
	// 0						0	-1/tan(fovy/2)	0
	// 0						c2	0				c1
	// 0						1	0				0
	// this is stored column major
	// NDC Z = c1/w + c2
	// c1 = (near*far)/(near-far)
	// c2 = far/(far-near)
	*matrix = {};
	matrix->m11 = 1.0f / (tanf(fov_y * 0.5f) * aspect_ratio);
	matrix->m32 = (far_plane / (far_plane - near_plane));
	matrix->m42 = 1.0f;
	matrix->m23 = -1.0f / tanf(fov_y * 0.5f);
	matrix->m34 = (near_plane * far_plane) / (near_plane - far_plane);
}

void matrix_4x4_translation(Matrix_4x4* matrix, Vec_3f translation)
{
	matrix->m11 = 1.0f;
	matrix->m21 = 0.0f;
	matrix->m31 = 0.0f;
	matrix->m41 = 0.0f;
	matrix->m12 = 0.0f;
	matrix->m22 = 1.0f;
	matrix->m32 = 0.0f;
	matrix->m42 = 0.0f;
	matrix->m13 = 0.0f;
	matrix->m23 = 0.0f;
	matrix->m33 = 1.0f;
	matrix->m43 = 0.0f;
	matrix->m14 = translation.x;
	matrix->m24 = translation.y;
	matrix->m34 = translation.z;
	matrix->m44 = 1.0f;
}

void matrix_4x4_rotation_x(Matrix_4x4* matrix, float32 r)
{
	float32 cr = cosf(r);
	float32 sr = sinf(r);
	matrix->m11 = 1.0f;
	matrix->m21 = 0.0f;
	matrix->m31 = 0.0f;
	matrix->m41 = 0.0f;
	matrix->m12 = 0.0f;
	matrix->m22 = cr;
	matrix->m32 = sr;
	matrix->m42 = 0.0f;
	matrix->m13 = 0.0f;
	matrix->m23 = -sr;
	matrix->m33 = cr;
	matrix->m43 = 0.0f;
	matrix->m14 = 0.0f;
	matrix->m24 = 0.0f;
	matrix->m34 = 0.0f;
	matrix->m44 = 1.0f;
}

void matrix_4x4_rotation_y(Matrix_4x4* matrix, float32 r)
{
	float32 cr = cosf(r);
	float32 sr = sinf(r);
	matrix->m11 = cr;
	matrix->m21 = 0.0f;
	matrix->m31 = -sr;
	matrix->m41 = 0.0f;
	matrix->m12 = 0.0f;
	matrix->m22 = 1.0f;
	matrix->m32 = 0.0f;
	matrix->m42 = 0.0f;
	matrix->m13 = sr;
	matrix->m23 = 0.0f;
	matrix->m33 = cr;
	matrix->m43 = 0.0f;
	matrix->m14 = 0.0f;
	matrix->m24 = 0.0f;
	matrix->m34 = 0.0f;
	matrix->m44 = 1.0f;
}

void matrix_4x4_rotation_z(Matrix_4x4* matrix, float32 r)
{
	float32 cr = cosf(r);
	float32 sr = sinf(r);
	matrix->m11 = cr;
	matrix->m21 = sr;
	matrix->m31 = 0.0f;
	matrix->m41 = 0.0f;
	matrix->m12 = -sr;
	matrix->m22 = cr;
	matrix->m32 = 0.0f;
	matrix->m42 = 0.0f;
	matrix->m13 = 0.0f;
	matrix->m23 = 0.0f;
	matrix->m33 = 1.0f;
	matrix->m43 = 0.0f;
	matrix->m14 = 0.0f;
	matrix->m24 = 0.0f;
	matrix->m34 = 0.0f;
	matrix->m44 = 1.0f;
}

void matrix_4x4_mul(Matrix_4x4* result, Matrix_4x4* a, Matrix_4x4* b)
{
	assert(result != a && result != b);
	result->m11 = (a->m11 * b->m11) + (a->m12 * b->m21) + (a->m13 * b->m31) + (a->m14 * b->m41);
	result->m21 = (a->m21 * b->m11) + (a->m22 * b->m21) + (a->m23 * b->m31) + (a->m24 * b->m41);
	result->m31 = (a->m31 * b->m11) + (a->m32 * b->m21) + (a->m33 * b->m31) + (a->m34 * b->m41);
	result->m41 = (a->m41 * b->m11) + (a->m42 * b->m21) + (a->m43 * b->m31) + (a->m44 * b->m41);
	result->m12 = (a->m11 * b->m12) + (a->m12 * b->m22) + (a->m13 * b->m32) + (a->m14 * b->m42);
	result->m22 = (a->m21 * b->m12) + (a->m22 * b->m22) + (a->m23 * b->m32) + (a->m24 * b->m42);
	result->m32 = (a->m31 * b->m12) + (a->m32 * b->m22) + (a->m33 * b->m32) + (a->m34 * b->m42);
	result->m42 = (a->m41 * b->m12) + (a->m42 * b->m22) + (a->m43 * b->m32) + (a->m44 * b->m42);
	result->m13 = (a->m11 * b->m13) + (a->m12 * b->m23) + (a->m13 * b->m33) + (a->m14 * b->m43);
	result->m23 = (a->m21 * b->m13) + (a->m22 * b->m23) + (a->m23 * b->m33) + (a->m24 * b->m43);
	result->m33 = (a->m31 * b->m13) + (a->m32 * b->m23) + (a->m33 * b->m33) + (a->m34 * b->m43);
	result->m43 = (a->m41 * b->m13) + (a->m42 * b->m23) + (a->m43 * b->m33) + (a->m44 * b->m43);
	result->m14 = (a->m11 * b->m14) + (a->m12 * b->m24) + (a->m13 * b->m34) + (a->m14 * b->m44);
	result->m24 = (a->m21 * b->m14) + (a->m22 * b->m24) + (a->m23 * b->m34) + (a->m24 * b->m44);
	result->m34 = (a->m31 * b->m14) + (a->m32 * b->m24) + (a->m33 * b->m34) + (a->m34 * b->m44);
	result->m44 = (a->m41 * b->m14) + (a->m42 * b->m24) + (a->m43 * b->m34) + (a->m44 * b->m44);
}

void matrix_4x4_lookat(Matrix_4x4* matrix, Vec_3f position, Vec_3f forward, Vec_3f up)
{
	Matrix_4x4 translation;
	matrix_4x4_translation(&translation, vec_3f_mul(position, -1.0f));

	Vec_3f project_up_onto_forward = vec_3f_mul(forward, vec_3f_dot(up, forward));
	Vec_3f view_up = vec_3f_normalised(vec_3f_sub(up, project_up_onto_forward));
	Vec_3f view_right = vec_3f_cross(forward, view_up);

	Matrix_4x4 rotation;
	rotation.m11 = view_right.x;
	rotation.m12 = view_right.y;
	rotation.m13 = view_right.z;
	rotation.m14 = 0.0f;
	rotation.m21 = forward.x;
	rotation.m22 = forward.y;
	rotation.m23 = forward.z;
	rotation.m24 = 0.0f;
	rotation.m31 = view_up.x;
	rotation.m32 = view_up.y;
	rotation.m33 = view_up.z;
	rotation.m34 = 0.0f;
	rotation.m41 = 0.0f;
	rotation.m42 = 0.0f;
	rotation.m43 = 0.0f;
	rotation.m44 = 1.0f;

	matrix_4x4_mul(matrix, &rotation, &translation);
}