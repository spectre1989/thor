#pragma once

#include "Core.h"
#include "Maths.h"
#include <Windows.h>



struct Graphics_State
{
	struct VkInstance_T* instance;
	struct VkPhysicalDevice_T* gpu;
	struct VkDevice_T* device;
	struct VkSwapchainKHR_T* swapchain;
	struct VkQueue_T* graphics_queue;
	struct VkQueue_T* present_queue;
	struct VkFence_T* fence;
	struct VkSemaphore_T* semaphore;
	struct VkCommandBuffer_T** command_buffers;
	struct VkDeviceMemory_T* uniform_buffer_memory;
	Matrix_4x4 projection_matrix;
};

void graphics_init(Graphics_State* graphics_state, HINSTANCE instance_handle, HWND window_handle, uint32 width, uint32 height, struct Linear_Allocator* allocator, Linear_Allocator* temp_allocator);
void graphics_draw(Graphics_State* graphics_state, Matrix_4x4* view_matrix, Vec_3f cube_position);