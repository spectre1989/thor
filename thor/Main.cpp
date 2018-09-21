#include "Bin_File.h"
#include "Geo_File.h"
#include "File.h"
#include "Memory.h"
#include "Pigg_File.h"
#include "String.h"
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <cstdio>



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

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugReportFlagsEXT                       /*flags*/,
    VkDebugReportObjectTypeEXT                  /*object_type*/,
    uint64_t                                    /*object*/,
    size_t                                      /*location*/,
    int32_t                                     /*message_code*/,
    const char*                                 layer_prefix,
    const char*                                 message,
    void*                                       /*user_data*/)
{
	char buffer[512];
	int32 message_length = snprintf(buffer, sizeof(buffer), "[%s]%s\n", layer_prefix, message);
	assert(message_length < sizeof(buffer));
	OutputDebugStringA(buffer);

	return VK_FALSE; // the caller shouldn't abort
}

static LRESULT CALLBACK window_callback(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	return DefWindowProcA(window_handle, msg, w_param, l_param);
}

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

		HWND window_handle = CreateWindowA(
		   window_class_name,
		   "Thor",				// lpWindowName
		   WS_OVERLAPPEDWINDOW, // dwStyle
		   200,					// x
		   100,					// y
		   1280,				// nWidth
		   800,					// nHeight
		   nullptr,				// hWndParent
		   nullptr,				// hMenu
		   instance_handle,
		   nullptr				// lpParam
		);

		ShowWindow(window_handle, SW_SHOW);

		Linear_Allocator temp_allocator;
		linear_allocator_create(&temp_allocator, megabytes(32));

		VkApplicationInfo application_info = {};
		application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application_info.pNext = nullptr;
		application_info.pApplicationName = "Thor";
		application_info.applicationVersion = 0;
		application_info.pEngineName = "Thor";
		application_info.engineVersion = 0;
		application_info.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pNext = nullptr;
		instance_info.flags = 0;
		instance_info.pApplicationInfo = &application_info;
		instance_info.enabledLayerCount = 0;
		instance_info.ppEnabledLayerNames = nullptr;
		const char* enabled_instance_extension_names[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		instance_info.enabledExtensionCount = sizeof(enabled_instance_extension_names) / sizeof(enabled_instance_extension_names[0]);
		instance_info.ppEnabledExtensionNames = enabled_instance_extension_names;

		VkInstance instance;
		VkResult result;

		result = vkCreateInstance(&instance_info, /*allocator*/ nullptr, &instance);
		assert(result == VK_SUCCESS);

		VkDebugReportCallbackCreateInfoEXT debug_callbacks_info = {};
		debug_callbacks_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debug_callbacks_info.pNext = nullptr;
		debug_callbacks_info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
									VK_DEBUG_REPORT_WARNING_BIT_EXT |
									VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
									VK_DEBUG_REPORT_ERROR_BIT_EXT |
									VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		debug_callbacks_info.pfnCallback = vulkan_debug_callback;
		debug_callbacks_info.pUserData = nullptr;

		// todo(jbr) define out callbacks in release
		VkDebugReportCallbackEXT debug_callbacks;
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		vkCreateDebugReportCallbackEXT(instance, &debug_callbacks_info, /*allocator*/ nullptr, &debug_callbacks);

		uint32 gpu_count;
		result = vkEnumeratePhysicalDevices(instance, &gpu_count, /*out_physical_devices*/nullptr);
		assert(result == VK_SUCCESS);
		assert(gpu_count);

		VkPhysicalDevice* gpus = (VkPhysicalDevice*)linear_allocator_alloc(&temp_allocator, sizeof(VkPhysicalDevice) * gpu_count);
		result = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus);
		assert(result == VK_SUCCESS);

		// for now just try to pick a dedicated gpu if available
		// todo(jbr) properly examine gpus and decide on best one
		VkPhysicalDevice gpu = nullptr;
		for (uint32 i = 0; i < gpu_count; ++i)
		{
			VkPhysicalDeviceProperties gpu_properties;
			vkGetPhysicalDeviceProperties(gpus[i], &gpu_properties);

			if (gpu_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				gpu = gpus[i];
				break;
			}
		}
		if (!gpu)
		{
			gpu = gpus[0];
		}
		
		uint32 queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, /*queue_families*/ nullptr);

		VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)linear_allocator_alloc(&temp_allocator, sizeof(VkQueueFamilyProperties) * queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families);

		uint32 chosen_queue_family_index = (uint32)-1;
		for (uint32 i = 0; i < queue_family_count; ++i)
		{
			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				chosen_queue_family_index = i;
				break;
			}
		}
		assert(chosen_queue_family_index != (uint32)-1);

		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.pNext = nullptr;
		queue_create_info.flags = 0;
		queue_create_info.queueFamilyIndex = chosen_queue_family_index;
		queue_create_info.queueCount = 1;
		float32 queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.pNext = nullptr;
		device_info.flags = 0;
		device_info.queueCreateInfoCount = 1;
		device_info.pQueueCreateInfos = &queue_create_info;
		device_info.enabledLayerCount = 0;
		device_info.ppEnabledLayerNames = nullptr;
		const char* enabled_device_extension_names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		device_info.enabledExtensionCount = sizeof(enabled_device_extension_names) / sizeof(enabled_device_extension_names[0]);
		device_info.ppEnabledExtensionNames = enabled_device_extension_names;
		device_info.pEnabledFeatures = nullptr;

		VkDevice device;
		result = vkCreateDevice(gpu, &device_info, /*allocator*/ nullptr, &device);
		assert(result == VK_SUCCESS);

		VkWin32SurfaceCreateInfoKHR surface_info = {};
		surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_info.pNext = nullptr;
		surface_info.flags = 0;
		surface_info.hinstance = instance_handle;
		surface_info.hwnd = window_handle;

		VkSurfaceKHR surface;
		result = vkCreateWin32SurfaceKHR(instance, &surface_info, /*allocator*/ nullptr, &surface);
		assert(result == VK_SUCCESS);

		VkSwapchainCreateInfoKHR swapchain_info = {};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.pNext = nullptr;
		swapchain_info.flags = 0;
		swapchain_info.surface = surface;

		/*typedef struct VkSwapchainCreateInfoKHR {
		VkStructureType                  sType;
		const void*                      pNext;
		VkSwapchainCreateFlagsKHR        flags;
		VkSurfaceKHR                     surface;
		uint32_t                         minImageCount;
		VkFormat                         imageFormat;
		VkColorSpaceKHR                  imageColorSpace;
		VkExtent2D                       imageExtent;
		uint32_t                         imageArrayLayers;
		VkImageUsageFlags                imageUsage;
		VkSharingMode                    imageSharingMode;
		uint32_t                         queueFamilyIndexCount;
		const uint32_t*                  pQueueFamilyIndices;
		VkSurfaceTransformFlagBitsKHR    preTransform;
		VkCompositeAlphaFlagBitsKHR      compositeAlpha;
		VkPresentModeKHR                 presentMode;
		VkBool32                         clipped;
		VkSwapchainKHR                   oldSwapchain;
	} VkSwapchainCreateInfoKHR;*/

		VkSwapchainKHR swapchain;
		result = vkCreateSwapchainKHR(device, &swapchain_info, /*allocator*/ nullptr, &swapchain);
		assert(result == VK_SUCCESS);

		while (true)
		{
			MSG msg;
			while (PeekMessageA(&msg, window_handle, 0 /*filter_min*/, 0 /*filter_max*/, PM_REMOVE)) // todo(jbr) make sure this only runs a max number of times
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
		}
	}

	return 0;
}

