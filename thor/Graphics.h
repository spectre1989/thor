#pragma once

#include "Core.h"
#include <Windows.h>



struct Graphics_State
{
	struct VkInstance_T* instance;
	struct VkPhysicalDevice_T* gpu;
	struct VkDevice_T* device;
};

void graphics_init(Graphics_State* graphics_state, HINSTANCE instance_handle, HWND window_handle, uint32 width, uint32 height, struct Linear_Allocator* temp_allocator);