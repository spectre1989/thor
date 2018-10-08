#include "Bin_File.h"
#include "Core.h"
#include "File.h"
#include "Geo_File.h"
#include "Graphics.h"
#include "Memory.h"
#include "Pigg_File.h"
#include "String.h"
#include <cmath>
#include <Windows.h>



struct Input_State
{
	bool32 keys[256];
};

static Input_State g_input_state;


static LRESULT CALLBACK window_callback(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		g_input_state.keys[(int32)w_param] = 1;
		break;

	case WM_KEYUP:
		g_input_state.keys[(int32)w_param] = 0;
		break;

	default:
		return DefWindowProcA(window_handle, msg, w_param, l_param);
	}

	return 0;
}

// todo(jbr) would it be better to use wall and disable selectively?
int CALLBACK WinMain(HINSTANCE instance_handle, HINSTANCE /*prev_instance_handle*/, LPSTR /*cmd_line*/, int /*cmd_show*/)
{
	const char* window_class_name = "Thor_Window_Class";

	WNDCLASSA window_class = {};
	window_class.style = 0;
	window_class.lpfnWndProc = window_callback;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance_handle;
	window_class.hIcon = nullptr;
	window_class.hCursor = nullptr;
	window_class.hbrBackground = nullptr;
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = window_class_name;

	RegisterClassA(&window_class);

	constexpr uint32 c_window_width = 1280;
	constexpr uint32 c_window_height = 800;

	HWND window_handle = CreateWindowA(
		window_class_name,
		"Thor",				// lpWindowName
		WS_OVERLAPPEDWINDOW,// dwStyle
		200,				// x
		100,				// y
		c_window_width,		// nWidth
		c_window_height,	// nHeight
		nullptr,			// hWndParent
		nullptr,			// hMenu
		instance_handle,
		nullptr				// lpParam
	);

	ShowWindow(window_handle, SW_SHOW);

	LARGE_INTEGER clock_frequency;
	QueryPerformanceFrequency(&clock_frequency);

	constexpr float32 c_target_frame_rate = 60.0f;
	constexpr float32 c_target_frame_dt = 1.0f / c_target_frame_rate;

	bool32 was_sleep_granularity_set = timeBeginPeriod(1) == TIMERR_NOERROR;

	Linear_Allocator allocator;
	linear_allocator_create(&allocator, megabytes(1));

	Linear_Allocator temp_allocator;
	linear_allocator_create(&temp_allocator, megabytes(1));

	Graphics_State* graphics_state = (Graphics_State*)linear_allocator_alloc(&allocator, sizeof(Graphics_State));
	graphics_init(graphics_state, instance_handle, window_handle, c_window_width, c_window_height, &allocator, &temp_allocator);

	Vec_3f camera_position = vec_3f(0.0f, 0.0f, 0.0f);
	Vec_3f camera_forward = vec_3f(0.0f, 1.0f, 0.0f);
	Vec_3f camera_right = vec_3f(1.0f, 0.0f, 0.0f);
	Matrix_4x4 view_matrix;
	Vec_3f camera_velocity = vec_3f(0.0f, 0.0f, 0.0f);

	while (true)
	{
		LARGE_INTEGER frame_start;
		QueryPerformanceCounter(&frame_start);

		MSG msg;
		while (PeekMessageA(&msg, window_handle, 0 /*filter_min*/, 0 /*filter_max*/, PM_REMOVE)) // todo(jbr) make sure this only runs a max number of times
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		constexpr float32 c_camera_speed = 5.0f;
		Vec_3f target_camera_velocity = vec_3f(0.0f, 0.0f, 0.0f);
		if (g_input_state.keys['W'])
		{
			target_camera_velocity = vec_3f_mul(camera_forward, c_camera_speed);
		}
		if (g_input_state.keys['S'])
		{
			target_camera_velocity = vec_3f_sub(target_camera_velocity, vec_3f_mul(camera_forward, c_camera_speed));
		}
		if (g_input_state.keys['A'])
		{
			target_camera_velocity = vec_3f_sub(target_camera_velocity, vec_3f_mul(camera_right, c_camera_speed));
		}
		if (g_input_state.keys['D'])
		{
			target_camera_velocity = vec_3f_add(target_camera_velocity, vec_3f_mul(camera_right, c_camera_speed));
		}
		if (g_input_state.keys['E'])
		{
			target_camera_velocity = vec_3f_add(target_camera_velocity, vec_3f(0.0f, 0.0f, c_camera_speed));
		}
		if (g_input_state.keys['Q'])
		{
			target_camera_velocity = vec_3f_sub(target_camera_velocity, vec_3f(0.0f, 0.0f, c_camera_speed));
		}

		target_camera_velocity = vec_3f_mul(vec_3f_normalised(target_camera_velocity), c_camera_speed);

		constexpr float32 c_camera_smoothing_factor = 12.0f;
		float32 smoothing_lerp_t = exp2(-c_camera_smoothing_factor * c_target_frame_dt);
		camera_velocity = vec_3f_lerp(target_camera_velocity, camera_velocity, smoothing_lerp_t);

		camera_position = vec_3f_add(camera_position, vec_3f_mul(camera_velocity, c_target_frame_dt));

		matrix_4x4_lookat(&view_matrix, camera_position, camera_forward, /*up*/ vec_3f(0.0f, 0.0f, 1.0f));

		graphics_draw(graphics_state, &view_matrix, /*cube_position*/ vec_3f(0.0f, 5.0f, 0.0f));

		// wait for end of frame
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);

		float32 frame_length = (now.QuadPart - frame_start.QuadPart) / (float32)clock_frequency.QuadPart;
		
		float32 frame_remaining = c_target_frame_dt - frame_length;

		if (was_sleep_granularity_set && frame_remaining >= 3.0f) // sleep can overlseep, so always spin for the last 2ms
		{
			DWORD sleep_for_ms = (DWORD)frame_remaining - 2;
			assert(sleep_for_ms);
			Sleep(sleep_for_ms);
		}

		while (true)
		{
			QueryPerformanceCounter(&now);
			frame_length = (now.QuadPart - frame_start.QuadPart) / (float32)clock_frequency.QuadPart;
			if (frame_length >= c_target_frame_dt)
			{
				break;
			}

			// at least try to do something useful while spinning
			if (PeekMessageA(&msg, window_handle, 0 /*filter_min*/, 0 /*filter_max*/, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
		}
	}

	return 0;
}

