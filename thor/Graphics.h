#pragma once

#include <Windows.h>



struct Graphics_State
{
	struct VkInstance_T* instance;
	struct VkDevice_T* device;
};

void graphics_init(Graphics_State* graphics_state, HINSTANCE instance_handle, HWND window_handle, struct Linear_Allocator* temp_allocator);