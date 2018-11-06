#pragma once

#include "Core.h"



constexpr float32 c_pi = 3.14159265f;
constexpr float32 c_deg_to_rad = c_pi / 180.0f;
constexpr float32 c_rad_to_deg = 1 / c_deg_to_rad;


struct Vec_3f
{
	float32 x, y, z;
};

struct Quat
{
	float32 zy, xz, yx, scalar;
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


float32 f32_clamp(float32 f, float32 min, float32 max);

Vec_3f vec_3f(float32 x, float32 y, float32 z);
Vec_3f vec_3f_add(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_sub(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_mul(Vec_3f v, float32 f);
Vec_3f vec_3f_normalised(Vec_3f v);
float vec_3f_dot(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_cross(Vec_3f a, Vec_3f b);
Vec_3f vec_3f_lerp(Vec_3f a, Vec_3f b, float32 t);

Quat quat(float32 zy, float32 xz, float32 yx, float32 scalar);
Quat quat_identity();
Quat quat_angle_axis(Vec_3f axis, float32 angle);
Vec_3f quat_mul(Quat q, Vec_3f v);
Quat quat_mul(Quat a, Quat b);

void matrix_4x4_projection(Matrix_4x4* matrix, float32 fov_y, float32 aspect_ratio, float32 near_plane, float32 far_plane);
void matrix_4x4_translation(Matrix_4x4* matrix, Vec_3f translation);
void matrix_4x4_mul(Matrix_4x4* result, Matrix_4x4* a, Matrix_4x4* b); // todo(jbr) should this be changed to be (a, b, result)? should really decide on a convention of param order
Vec_3f matrix_4x4_mul_direction(Matrix_4x4* matrix, Vec_3f v);
void matrix_4x4_camera(Matrix_4x4* matrix, Vec_3f position, Vec_3f forward, Vec_3f up);
void matrix_4x4_lookat(Matrix_4x4* matrix, Vec_3f position, Vec_3f target, Vec_3f up);
void matrix_4x4_rotation_x(Matrix_4x4* matrix, float32 r);
void matrix_4x4_rotation_y(Matrix_4x4* matrix, float32 r);
void matrix_4x4_rotation_z(Matrix_4x4* matrix, float32 r);