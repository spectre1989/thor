#include "Bin_File.h"
#include "Core.h"
#include "File.h"
#include "Geo_File.h"
#include "Graphics.h"
#include "Memory.h"
#include "Pigg_File.h"
#include "String.h"
#include <Windows.h>



struct Input_State
{
	bool32 keys[256];
};

static Input_State g_input_state;


static void on_pigg_file_found(const char* path, void* state)
{
	Linear_Allocator* allocator = (Linear_Allocator*)state;
	linear_allocator_reset(allocator);

	unpack_pigg_file(path, allocator);
}

static void on_bin_file_found(const char* path, void* /*state*/)
{
	File_Handle bin_file = file_open_read(path);
	bin_file_check(bin_file);
	file_close(bin_file);
}

static void on_geo_file_found(const char* path, void* state)
{
	Linear_Allocator* allocator = (Linear_Allocator*)state;
	linear_allocator_reset(allocator);

	File_Handle geo_file = file_open_read(path);
	geo_file_check(geo_file, allocator);
	file_close(geo_file);
}

static LRESULT CALLBACK window_callback(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		g_input_state.keys[(int32)w_param] = 1;
		return 0;
		break;

	case WM_KEYUP:
		g_input_state.keys[(int32)w_param] = 0;
		return 0;
		break;
	}

	return DefWindowProcA(window_handle, msg, w_param, l_param);
}

// todo(jbr) would it be better to use wall and disable selectively?
int CALLBACK WinMain(HINSTANCE instance_handle, HINSTANCE /*prev_instance_handle*/, LPSTR cmd_line, int /*cmd_show*/)
{
	bool unpack_piggs = false;
	bool check_bins = false;
	bool check_geos = false;
	bool run_map_viewer = true;


	if (unpack_piggs)
	{
		Linear_Allocator allocator = {};
		linear_allocator_create(&allocator, megabytes(256));

		file_search(cmd_line, "*.pigg", on_pigg_file_found, /*state*/&allocator);

		linear_allocator_destroy(&allocator);
	}

	if (check_bins)
	{
		file_search(cmd_line, "*.*", on_bin_file_found, /*state*/nullptr);
	}

	if (check_geos)
	{
		Linear_Allocator allocator = {};
		linear_allocator_create(&allocator, megabytes(32));

		file_search(cmd_line, "*.geo", on_geo_file_found, /*state*/&allocator);

		linear_allocator_destroy(&allocator);
	}

	if (run_map_viewer)
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

		while (true)
		{
			MSG msg;
			while (PeekMessageA(&msg, window_handle, 0 /*filter_min*/, 0 /*filter_max*/, PM_REMOVE)) // todo(jbr) make sure this only runs a max number of times
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}

			float32 speed = 0.01f;
			if (g_input_state.keys['W'])
			{
				camera_position = vec_3f_add(camera_position, vec_3f_mul(camera_forward, speed));
			}
			if (g_input_state.keys['S'])
			{
				camera_position = vec_3f_sub(camera_position, vec_3f_mul(camera_forward, speed));
			}
			if (g_input_state.keys['A'])
			{
				camera_position = vec_3f_sub(camera_position, vec_3f_mul(camera_right, speed));
			}
			if (g_input_state.keys['D'])
			{
				camera_position = vec_3f_add(camera_position, vec_3f_mul(camera_right, speed));
			}
			if (g_input_state.keys['E'])
			{
				camera_position = vec_3f_add(camera_position, vec_3f(0.0f, 0.0f, speed));
			}
			if (g_input_state.keys['Q'])
			{
				camera_position = vec_3f_sub(camera_position, vec_3f(0.0f, 0.0f, speed));
			}


			matrix_4x4_lookat(&view_matrix, camera_position, camera_forward, /*up*/ vec_3f(0.0f, 0.0f, 1.0f));

			graphics_draw(graphics_state, &view_matrix, /*cube_position*/ vec_3f(0.0f, 5.0f, 0.0f));
		}
	}

	return 0;
}

