#include "Graphics.h"

#include "Bin_File.h" // todo(jbr) shouldn't have this dependency
#include "File.h"
#include "Memory.h"
#include <vulkan/vulkan.h>
#include <cstdio>



constexpr int32 c_bytes_per_transform = sizeof(float32) * 4 * 4;



static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  /*object_type*/,
    uint64_t                                    /*object*/,
    size_t                                      /*location*/,
    int32_t                                     /*message_code*/,
    const char*                                 layer_prefix,
    const char*                                 message,
    void*                                       /*user_data*/)
{
	if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT ||
		flags == VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ||
		flags == VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		DebugBreak();
	}

	char buffer[1024];
	int32 message_length = snprintf(buffer, sizeof(buffer), "[%s]%s\n", layer_prefix, message);
	assert(message_length < sizeof(buffer));
	OutputDebugStringA(buffer);

	return VK_FALSE; // the caller shouldn't abort
}

static uint32 get_memory_type_index(VkPhysicalDeviceMemoryProperties* gpu_memory_properties, uint32 supported_memory_type_bits, VkMemoryPropertyFlags required_properties)
{
	for (uint32 i = 0; i < gpu_memory_properties->memoryTypeCount; ++i)
	{
		if ((1 << i) & supported_memory_type_bits &&
			(gpu_memory_properties->memoryTypes[i].propertyFlags & required_properties) == required_properties)
		{
			return i;
		}
	}

	return (uint32)-1;
}

static VkShaderModule create_shader_module(VkDevice device, const char* shader_file_path, Linear_Allocator* temp_allocator)
{
	File_Handle shader_file = file_open_read(shader_file_path);
	uint32 shader_file_size = file_size(shader_file);
	uint8* shader_bytes = linear_allocator_alloc(temp_allocator, shader_file_size);
	file_read(shader_file, shader_file_size, /*out*/ shader_bytes);

	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.pNext = nullptr;
	shader_module_info.flags = 0;
	shader_module_info.codeSize = shader_file_size;
	shader_module_info.pCode = (uint32*)shader_bytes;

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device, &shader_module_info, /*allocator*/ nullptr, &shader_module);
	assert(result == VK_SUCCESS);

	return shader_module;
}

static void create_buffer(VkDevice device, 
	VkPhysicalDeviceMemoryProperties* gpu_memory_properties, 
	VkDeviceSize size, 
	VkBufferUsageFlags usage_flags, 
	VkMemoryPropertyFlags required_memory_properties,
	VkBuffer* out_buffer,
	VkDeviceMemory* out_buffer_memory)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = nullptr;
	buffer_info.flags = 0;
	buffer_info.size = size;
	buffer_info.usage = usage_flags;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.queueFamilyIndexCount = 0;
	buffer_info.pQueueFamilyIndices = nullptr;

	VkBuffer buffer;
	VkResult result = vkCreateBuffer(device, &buffer_info, /*allocator*/ nullptr, &buffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkMemoryAllocateInfo mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = nullptr;
	mem_alloc_info.allocationSize = memory_requirements.size;
	mem_alloc_info.memoryTypeIndex = get_memory_type_index(gpu_memory_properties, memory_requirements.memoryTypeBits, required_memory_properties);
	assert(mem_alloc_info.memoryTypeIndex != (uint32)-1);

	VkDeviceMemory buffer_memory;
	result = vkAllocateMemory(device, &mem_alloc_info, /*allocator*/ nullptr, &buffer_memory);
	assert(result == VK_SUCCESS);

	result = vkBindBufferMemory(device, buffer, buffer_memory, /*offset*/ 0);
	assert(result == VK_SUCCESS);

	*out_buffer = buffer;
	*out_buffer_memory = buffer_memory;
}

static void create_quad(Vec_3f pos, Vec_3f right, Vec_3f up, float* vertices, int32 vertex_offset, uint16* indices, int32 index_offset)
{
	Vec_3f half_size = vec_3f_mul(vec_3f_add(right, up), 0.5f);

	Vec_3f bottom_left = vec_3f_add(pos, vec_3f_mul(half_size, -1.0f));
	Vec_3f top_left = vec_3f_add(bottom_left, up);
	Vec_3f top_right = vec_3f_add(top_left, right);
	Vec_3f bottom_right = vec_3f_add(bottom_left, right);

	int32 vertex_i = vertex_offset * 3;

	vertices[vertex_i++] = bottom_left.x;
	vertices[vertex_i++] = bottom_left.y;
	vertices[vertex_i++] = bottom_left.z;
	vertices[vertex_i++] = top_left.x;
	vertices[vertex_i++] = top_left.y;
	vertices[vertex_i++] = top_left.z;
	vertices[vertex_i++] = top_right.x;
	vertices[vertex_i++] = top_right.y;
	vertices[vertex_i++] = top_right.z;
	vertices[vertex_i++] = bottom_right.x;
	vertices[vertex_i++] = bottom_right.y;
	vertices[vertex_i++] = bottom_right.z;

	indices[index_offset++] = uint16(vertex_offset);
	indices[index_offset++] = uint16(vertex_offset + 2);
	indices[index_offset++] = uint16(vertex_offset + 1);
	indices[index_offset++] = uint16(vertex_offset);
	indices[index_offset++] = uint16(vertex_offset + 3);
	indices[index_offset++] = uint16(vertex_offset + 2);
}

void graphics_init(Graphics_State* graphics_state, HINSTANCE instance_handle, HWND window_handle, uint32 width, uint32 height, int32 model_count, Model* models, int32* instance_count, Transform** instances, Linear_Allocator* allocator, Linear_Allocator* temp_allocator)
{
	*graphics_state = {};

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

	VkResult result;

	result = vkCreateInstance(&instance_info, /*allocator*/ nullptr, &graphics_state->instance);
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
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(graphics_state->instance, "vkCreateDebugReportCallbackEXT");
	vkCreateDebugReportCallbackEXT(graphics_state->instance, &debug_callbacks_info, /*allocator*/ nullptr, &debug_callbacks);

	VkWin32SurfaceCreateInfoKHR surface_info = {};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = nullptr;
	surface_info.flags = 0;
	surface_info.hinstance = instance_handle;
	surface_info.hwnd = window_handle;

	VkSurfaceKHR surface;
	result = vkCreateWin32SurfaceKHR(graphics_state->instance, &surface_info, /*allocator*/ nullptr, &surface);
	assert(result == VK_SUCCESS);

	uint32 gpu_count;
	result = vkEnumeratePhysicalDevices(graphics_state->instance, &gpu_count, /*out_physical_devices*/nullptr);
	assert(result == VK_SUCCESS);
	assert(gpu_count);

	VkPhysicalDevice* gpus = (VkPhysicalDevice*)linear_allocator_alloc(temp_allocator, sizeof(VkPhysicalDevice) * gpu_count);
	result = vkEnumeratePhysicalDevices(graphics_state->instance, &gpu_count, gpus);
	assert(result == VK_SUCCESS);

	// for now just try to pick a dedicated gpu if available
	// todo(jbr) properly examine gpus and decide on best one
	VkPhysicalDeviceProperties gpu_properties = {};
	for (uint32 i = 0; i < gpu_count; ++i)
	{
		vkGetPhysicalDeviceProperties(gpus[i], &gpu_properties);

		if (gpu_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			graphics_state->gpu = gpus[i];
			break;
		}
	}
	if (!graphics_state->gpu)
	{
		vkGetPhysicalDeviceProperties(gpus[0], &gpu_properties);
		graphics_state->gpu = gpus[0];
	}
		
	uint32 queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(graphics_state->gpu, &queue_family_count, /*queue_families*/ nullptr);

	VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)linear_allocator_alloc(temp_allocator, sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(graphics_state->gpu, &queue_family_count, queue_families);

	uint32 graphics_queue_family_index = (uint32)-1;
	uint32 present_queue_family_index = (uint32)-1;
	for (uint32 i = 0; i < queue_family_count; ++i)
	{
		VkBool32 present_supported;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(graphics_state->gpu, i, surface, &present_supported);
		assert(result == VK_SUCCESS);

		uint32 graphics_supported = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

		if (graphics_supported && present_supported)
		{
			graphics_queue_family_index = i;
			present_queue_family_index = i;
			break;
		}
		else
		{
			if (graphics_queue_family_index == (uint32)-1 && graphics_supported)
			{
				graphics_queue_family_index = i;
			}
			if (present_queue_family_index == (uint32)-1 && present_supported)
			{
				present_queue_family_index = i;
			}
		}
	}
	assert(graphics_queue_family_index != (uint32)-1 && present_queue_family_index != (uint32)-1);

	VkDeviceQueueCreateInfo queue_create_info[2];
	queue_create_info[0] = {};
	queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info[0].pNext = nullptr;
	queue_create_info[0].flags = 0;
	queue_create_info[0].queueFamilyIndex = graphics_queue_family_index;
	queue_create_info[0].queueCount = 1;
	float32 queue_priority = 1.0f;
	queue_create_info[0].pQueuePriorities = &queue_priority;
	queue_create_info[1] = {};
	queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info[1].pNext = nullptr;
	queue_create_info[1].flags = 0;
	queue_create_info[1].queueFamilyIndex = graphics_queue_family_index;
	queue_create_info[1].queueCount = 1;
	queue_create_info[1].pQueuePriorities = &queue_priority;
		
	uint32 chosen_queue_count = graphics_queue_family_index == present_queue_family_index ? 1 : 2;

	VkPhysicalDeviceFeatures gpu_features_to_enable = {};
	gpu_features_to_enable.fillModeNonSolid = true; // for wireframe drawing

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = nullptr;
	device_info.flags = 0;
	device_info.queueCreateInfoCount = chosen_queue_count;
	device_info.pQueueCreateInfos = queue_create_info;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = nullptr;
	const char* enabled_device_extension_names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	device_info.enabledExtensionCount = sizeof(enabled_device_extension_names) / sizeof(enabled_device_extension_names[0]);
	device_info.ppEnabledExtensionNames = enabled_device_extension_names;
	device_info.pEnabledFeatures = &gpu_features_to_enable;

	result = vkCreateDevice(graphics_state->gpu, &device_info, /*allocator*/ nullptr, &graphics_state->device);
	assert(result == VK_SUCCESS);

	vkGetDeviceQueue(graphics_state->device, graphics_queue_family_index, /*queue_index*/ 0, &graphics_state->graphics_queue);
	vkGetDeviceQueue(graphics_state->device, present_queue_family_index, /*queue_index*/ 0, &graphics_state->present_queue);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics_state->gpu, surface, &surface_capabilities);
	assert(result == VK_SUCCESS);

	uint32 surface_format_count;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_state->gpu, surface, &surface_format_count, /*surface_formats*/ nullptr);
	assert(result == VK_SUCCESS);

	VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)linear_allocator_alloc(temp_allocator, sizeof(VkSurfaceFormatKHR) * surface_format_count);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_state->gpu, surface, &surface_format_count, surface_formats);
	assert(result == VK_SUCCESS);

	VkFormat swapchain_image_format = surface_formats[0].format != VK_FORMAT_UNDEFINED ? surface_formats[0].format : VK_FORMAT_B8G8R8A8_UNORM; // todo(jbr) pick best image format/colourspace

	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = nullptr;
	swapchain_info.flags = 0;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = surface_capabilities.minImageCount;
	swapchain_info.imageFormat = swapchain_image_format;
	swapchain_info.imageColorSpace = surface_formats[0].colorSpace;
	if (surface_capabilities.currentExtent.width == 0xffffffff)
	{
		if (width < surface_capabilities.minImageExtent.width ||
			height < surface_capabilities.minImageExtent.height)
		{
			swapchain_info.imageExtent = surface_capabilities.minImageExtent;
		}
		else if (width > surface_capabilities.maxImageExtent.width ||
				height > surface_capabilities.maxImageExtent.height)
		{
			swapchain_info.imageExtent = surface_capabilities.maxImageExtent;
		}
		else
		{
			swapchain_info.imageExtent.width = width;
			swapchain_info.imageExtent.height = height;
		}
	}
	else
	{
		swapchain_info.imageExtent = surface_capabilities.currentExtent;
	}
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	uint32 chosen_queues[2] = { graphics_queue_family_index, present_queue_family_index };
	if (chosen_queue_count == 1)
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;		// only need to specify these if sharing mode is concurrent
		swapchain_info.pQueueFamilyIndices = nullptr;
	}
	else
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // todo(jbr) better to transfer ownership explicitly?
		swapchain_info.queueFamilyIndexCount = chosen_queue_count;
		swapchain_info.pQueueFamilyIndices = chosen_queues;
	}
	swapchain_info.preTransform = surface_capabilities.currentTransform;
	VkCompositeAlphaFlagBitsKHR composite_alpha_modes[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	VkCompositeAlphaFlagBitsKHR composite_alpha = (VkCompositeAlphaFlagBitsKHR)-1;
	for (int32 i = 0; i < 4; ++i)
	{
		if (surface_capabilities.supportedCompositeAlpha & composite_alpha_modes[i])
		{
			composite_alpha = composite_alpha_modes[i];
			break;
		}
	}
	assert(composite_alpha != (VkCompositeAlphaFlagBitsKHR)-1);
	swapchain_info.compositeAlpha = composite_alpha;
	swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be supported
	swapchain_info.clipped = true;
	swapchain_info.oldSwapchain = nullptr;

	result = vkCreateSwapchainKHR(graphics_state->device, &swapchain_info, /*allocator*/ nullptr, &graphics_state->swapchain);
	assert(result == VK_SUCCESS);

	uint32 swapchain_image_count;
	result = vkGetSwapchainImagesKHR(graphics_state->device, graphics_state->swapchain, &swapchain_image_count, /*swapchain_images*/ nullptr);
	assert(result == VK_SUCCESS);

	VkImage* swapchain_images = (VkImage*)linear_allocator_alloc(temp_allocator, sizeof(VkImage) * swapchain_image_count);
	result = vkGetSwapchainImagesKHR(graphics_state->device, graphics_state->swapchain, &swapchain_image_count, swapchain_images);
	assert(result == VK_SUCCESS);

	VkImageView* swapchain_image_views = (VkImageView*)linear_allocator_alloc(temp_allocator, sizeof(VkImageView) * swapchain_image_count);
	VkImageViewCreateInfo image_view_info = {};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.pNext = nullptr;
	image_view_info.flags = 0;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = swapchain_image_format;
	image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;
	for (uint32 i = 0; i < swapchain_image_count; ++i)
	{
		image_view_info.image = swapchain_images[i];
		result = vkCreateImageView(graphics_state->device, &image_view_info, /*allocator*/ nullptr, &swapchain_image_views[i]);
		assert(result == VK_SUCCESS);
	}

	VkImageCreateInfo depth_buffer_info = {};
	depth_buffer_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth_buffer_info.pNext = nullptr;
	depth_buffer_info.flags = 0;
	depth_buffer_info.imageType = VK_IMAGE_TYPE_2D;
	const VkFormat c_depth_buffer_format = VK_FORMAT_D32_SFLOAT;
	depth_buffer_info.format = c_depth_buffer_format;
	depth_buffer_info.extent.width = swapchain_info.imageExtent.width;
	depth_buffer_info.extent.height = swapchain_info.imageExtent.height;
	depth_buffer_info.extent.depth = 1;
	depth_buffer_info.mipLevels = 1;
	depth_buffer_info.arrayLayers = 1;
	depth_buffer_info.samples = VK_SAMPLE_COUNT_1_BIT; // todo(jbr) multisampling depth buffers?
	depth_buffer_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	depth_buffer_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depth_buffer_info.queueFamilyIndexCount = 0;
	depth_buffer_info.pQueueFamilyIndices = nullptr;
	depth_buffer_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage depth_buffer;
	result = vkCreateImage(graphics_state->device, &depth_buffer_info, /*allocator*/ nullptr, &depth_buffer);
	assert(result == VK_SUCCESS);

	VkPhysicalDeviceMemoryProperties gpu_memory_properties = {};
	vkGetPhysicalDeviceMemoryProperties(graphics_state->gpu, &gpu_memory_properties);

	VkMemoryRequirements depth_buffer_memory_requirements = {};
	vkGetImageMemoryRequirements(graphics_state->device, depth_buffer, &depth_buffer_memory_requirements);

	VkMemoryAllocateInfo mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = nullptr;
	mem_alloc_info.allocationSize = depth_buffer_memory_requirements.size;
	mem_alloc_info.memoryTypeIndex = get_memory_type_index(&gpu_memory_properties, depth_buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	assert(mem_alloc_info.memoryTypeIndex != (uint32)-1);

	VkDeviceMemory depth_buffer_mem;
	result = vkAllocateMemory(graphics_state->device, &mem_alloc_info, /*allocator*/ nullptr, &depth_buffer_mem);
	assert(result == VK_SUCCESS);

	result = vkBindImageMemory(graphics_state->device, depth_buffer, depth_buffer_mem, /*offset*/ 0);
	assert(result == VK_SUCCESS);

	image_view_info = {};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.pNext = nullptr;
	image_view_info.flags = 0;
	image_view_info.image = depth_buffer;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = depth_buffer_info.format;
	image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;
	
	VkImageView depth_buffer_image_view;
	result = vkCreateImageView(graphics_state->device, &image_view_info, /*allocator*/ nullptr, &depth_buffer_image_view);
	assert(result == VK_SUCCESS);

	int32 max_transforms_per_ubo = gpu_properties.limits.maxUniformBufferRange / c_bytes_per_transform;

	int32 total_transform_count = 0;
	for (int32 model_i = 0; model_i < model_count; ++model_i)
	{
		total_transform_count += instance_count[model_i];
	}

	Matrix_4x4* transforms = (Matrix_4x4*)linear_allocator_alloc(allocator, sizeof(Matrix_4x4) * total_transform_count);
	
	Matrix_4x4* transform_iter = transforms;
	for (int32 model_i = 0; model_i < model_count; ++model_i)
	{
		for (int32 instance_i = 0; instance_i < instance_count[model_i]; ++instance_i)
		{
			matrix_4x4_transform(transform_iter, instances[model_i][instance_i].position, instances[model_i][instance_i].rotation);
			++transform_iter;
		}
	}

	int32 ubo_count = (total_transform_count / max_transforms_per_ubo) + (total_transform_count % max_transforms_per_ubo ? 1 : 0);
	
	graphics_state->ubos = (UBO*)linear_allocator_alloc(allocator, sizeof(UBO) * ubo_count);
	graphics_state->ubo_count = ubo_count;

	int32 model_i = 0;
	int32 instances_remaining = instance_count[model_i];
	for (int32 ubo_i = 0; ubo_i < ubo_count; ++ubo_i)
	{
		UBO* ubo = &graphics_state->ubos[ubo_i];
		*ubo = {};

		ubo->world_transforms = &transforms[ubo_i * max_transforms_per_ubo];

		int32 space_in_ubo = max_transforms_per_ubo;
		while (space_in_ubo)
		{
			UBO::Model_Info* model_info = (UBO::Model_Info*)linear_allocator_alloc(allocator, sizeof(UBO::Model_Info));
			
			model_info->instance_count = i32_min(space_in_ubo, instances_remaining);
			// todo(jbr) vertex buffer, index buffer, index count

			++ubo->model_count;
			ubo->transform_count += model_info->instance_count;

			space_in_ubo -= model_info->instance_count;
			instances_remaining -= model_info->instance_count;

			if (!instances_remaining)
			{
				++model_i;
				assert(model_i < ubo->model_count);
				instances_remaining = instance_count[model_i];
			}
		}

		// todo(jbr) device_memory_offset, descriptor_set
	}
	


	/* 
	old shit that used to be in geobin loading 
	
	char geo_base_path[256];
	string_concat(geo_base_path, sizeof(geo_base_path), coh_data_path, "/");

	int32 total_model_count = 0;
	Geo* geo = geos;
	while (geo)
	{
		Geo_Model* geo_model = geo->models;
		while (geo_model)
		{
			++total_model_count;

			geo_model = geo_model->next;
		}

		geo = geo->next;
	}

	Model* models = (Model*)linear_allocator_alloc(model_allocator, sizeof(Model) * total_model_count);
	Model_Instances* model_instances = (Model_Instances*)linear_allocator_alloc(model_instance_allocator, sizeof(Model_Instances) * total_model_count);
	
	// as we go write models/model instances here
	Model* current_model = models;
	Model_Instances* current_model_instances = model_instances;
	
	geo = geos;
	while (geo)
	{
		// reset the geo temp allocator for each file
		Linear_Allocator geo_temp_allocator = *temp_allocator;

		// count the models we want from this geo so we can make an array of model names
		Geo_Model* geo_model = geo->models;
		int32 model_count = 0;
		while (geo_model)
		{
			++model_count;
			geo_model = geo_model->next;
		}

		// make array of model names
		const char** model_names = (const char**)linear_allocator_alloc(&geo_temp_allocator, sizeof(const char*) * model_count);
		geo_model = geo->models;
		int32 model_i = 0;
		while (geo_model)
		{
			model_names[model_i++] = geo_model->name;
			geo_model = geo_model->next;
		}

		// read models from geo file
		char geo_file_path[256];
		string_concat(geo_file_path, sizeof(geo_file_path), geo_base_path, geo->relative_file_path);

		File_Handle geo_file = file_open_read(geo_file_path);
		geo_file_read(geo_file, model_names, current_model, model_count, model_allocator, &geo_temp_allocator);
		current_model += model_count;
		file_close(geo_file);
		
		// convert instance position/rotation 
		geo_model = geo->models;
		while (geo_model)
		{
			*current_model_instances = {};

			Model_Instance* instance = geo_model->instances;
			while (instance)
			{
				++current_model_instances->transform_count;
				instance = instance->next;
			}

			current_model_instances->transforms = (Matrix_4x4*)linear_allocator_alloc(model_instance_allocator, sizeof(Matrix_4x4) * current_model_instances->transform_count);

			Matrix_4x4* current_transform = current_model_instances->transforms;
			instance = geo_model->instances;
			while (instance)
			{
				matrix_4x4_transform(current_transform, instance->position, instance->rotation);
				++current_transform;

				instance = instance->next;
			}

			++current_model_instances;

			geo_model = geo_model->next;
		}

		geo = geo->next;
	}

	*out_model_count = total_model_count;
	*out_model_instances = model_instances;
	*/
	
	VkBuffer uniform_buffer;
	create_buffer(graphics_state->device, 
		&gpu_memory_properties, 
		ubo_size, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		&uniform_buffer, 
		&graphics_state->uniform_buffer_memory);

	VkDescriptorSetLayoutBinding layout_binding = {};
	layout_binding.binding = 0;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.pNext = nullptr;
	descriptor_set_layout_info.flags = 0;
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &layout_binding;

	VkDescriptorSetLayout descriptor_set_layout;
	result = vkCreateDescriptorSetLayout(graphics_state->device, &descriptor_set_layout_info, /*allocator*/ nullptr, &descriptor_set_layout);
	assert(result == VK_SUCCESS);

	VkPushConstantRange push_constant_range = {};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(int32);
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext = nullptr;
	pipeline_layout_info.flags = 0;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	VkPipelineLayout pipeline_layout;
	result = vkCreatePipelineLayout(graphics_state->device, &pipeline_layout_info, /*allocator*/ nullptr, &pipeline_layout);
	assert(result == VK_SUCCESS);

	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_info = {};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.pNext = nullptr;
	descriptor_pool_info.flags = 0;
	descriptor_pool_info.maxSets = 1;
	descriptor_pool_info.poolSizeCount = 1;
	descriptor_pool_info.pPoolSizes = &descriptor_pool_size;

	VkDescriptorPool descriptor_pool;
	result = vkCreateDescriptorPool(graphics_state->device, &descriptor_pool_info, /*allocator*/ nullptr, &descriptor_pool);
	assert(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
	descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_alloc_info.pNext = nullptr;
	descriptor_set_alloc_info.descriptorPool = descriptor_pool;
	descriptor_set_alloc_info.descriptorSetCount = 1;
	descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

	VkDescriptorSet descriptor_set;
	result = vkAllocateDescriptorSets(graphics_state->device, &descriptor_set_alloc_info, &descriptor_set);
	assert(result == VK_SUCCESS);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = uniform_buffer;
	buffer_info.offset = 0;
	buffer_info.range = ubo_size;

	VkWriteDescriptorSet descriptor_set_write = {};
	descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_set_write.pNext = nullptr;
	descriptor_set_write.dstSet = descriptor_set;
	descriptor_set_write.dstBinding = 0;
	descriptor_set_write.dstArrayElement = 0;
	descriptor_set_write.descriptorCount = 1;
	descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_set_write.pImageInfo = nullptr;
	descriptor_set_write.pBufferInfo = &buffer_info;
	descriptor_set_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(graphics_state->device, /*write_count*/ 1, &descriptor_set_write, /*copy_count*/ 0, /*copies*/ nullptr);

	VkAttachmentDescription attachments[2];
	attachments[0] = {};
	attachments[0].flags = 0;
	attachments[0].format = swapchain_image_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT; // todo(jbr) is this AA?
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[1] = {};
	attachments[1].flags = 0;
	attachments[1].format = c_depth_buffer_format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT; // todo(jbr) is this AA?
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colour_attachment_reference = {};
	colour_attachment_reference.attachment = 0;
	colour_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_reference = {};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colour_attachment_reference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depth_attachment_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.pNext = nullptr;
	render_pass_info.flags = 0;
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 0;
	render_pass_info.pDependencies = nullptr;

	VkRenderPass render_pass;
	result = vkCreateRenderPass(graphics_state->device, &render_pass_info, /*allocator*/ nullptr, &render_pass);
	assert(result == VK_SUCCESS);

	VkFramebuffer* framebuffers = (VkFramebuffer*)linear_allocator_alloc(temp_allocator, sizeof(VkFramebuffer) * swapchain_image_count); // todo(jbr) make sure anything actually stored comes from a permanent allocator

	VkImageView framebuffer_attachments[2];
	framebuffer_attachments[1] = depth_buffer_image_view;

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.pNext = nullptr;
	framebuffer_info.flags = 0;
	framebuffer_info.renderPass = render_pass;
	framebuffer_info.attachmentCount = 2;
	framebuffer_info.pAttachments = framebuffer_attachments;
	framebuffer_info.width = swapchain_info.imageExtent.width;
	framebuffer_info.height = swapchain_info.imageExtent.height;
	framebuffer_info.layers = 1;

	for (uint32 i = 0; i < swapchain_image_count; ++i)
	{
		framebuffer_attachments[0] = swapchain_image_views[i];

		vkCreateFramebuffer(graphics_state->device, &framebuffer_info, /*allocator*/ nullptr, &framebuffers[i]);
	}

	constexpr int32 c_num_faces = 6;
	constexpr int32 c_vertex_count = 4 * c_num_faces;
	constexpr int32 c_float32_count_per_vertex = 3;
	constexpr int32 c_vertex_float32_count = c_vertex_count * c_float32_count_per_vertex;
	constexpr int32 c_vbo_size = c_vertex_float32_count * sizeof(float32);
	constexpr int32 c_index_count = 6 * c_num_faces;
	constexpr int32 c_ibo_size = c_index_count * sizeof(uint16);
	
	float32 vertices[c_vertex_float32_count];
	uint16 indices[c_index_count];

	int32 vertex_offset = 0;
	int32 index_offset = 0;
	// front face
	create_quad(vec_3f(0.0f, -0.5f, 0.0f), // pos
				vec_3f(1.0f, 0.0f, 0.0f),  // right
				vec_3f(0.0f, 0.0f, 1.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	// back face
	create_quad(vec_3f(0.0f, 0.5f, 0.0f), // pos
				vec_3f(-1.0f, 0.0f, 0.0f),  // right
				vec_3f(0.0f, 0.0f, 1.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	// left face
	create_quad(vec_3f(-0.5f, 0.0f, 0.0f), // pos
				vec_3f(0.0f, -1.0f, 0.0f),  // right
				vec_3f(0.0f, 0.0f, 1.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	// left face
	create_quad(vec_3f(0.5f, 0.0f, 0.0f), // pos
				vec_3f(0.0f, 1.0f, 0.0f),  // right
				vec_3f(0.0f, 0.0f, 1.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	// top face
	create_quad(vec_3f(0.0f, 0.0f, 0.5f), // pos
				vec_3f(1.0f, 0.0f, 0.0f),  // right
				vec_3f(0.0f, 1.0f, 0.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	// bottom face
	create_quad(vec_3f(0.0f, 0.0f, -0.5f), // pos
				vec_3f(1.0f, 0.0f, 0.0f),  // right
				vec_3f(0.0f, -1.0f, 0.0f),  // up
				vertices, vertex_offset, indices, index_offset);
	vertex_offset += 4;
	index_offset += 6;

	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	create_buffer(graphics_state->device,
		&gpu_memory_properties,
		c_vbo_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vertex_buffer,
		&vertex_buffer_memory);

	float* vbo_data;
	result = vkMapMemory(graphics_state->device, vertex_buffer_memory, /*offset*/ 0, c_vbo_size, /*flags*/ 0, (void**)&vbo_data);
	assert(result == VK_SUCCESS);
	memcpy(vbo_data, vertices, c_vbo_size);
	vkUnmapMemory(graphics_state->device, vertex_buffer_memory);

	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	create_buffer(graphics_state->device,
		&gpu_memory_properties,
		c_ibo_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&index_buffer,
		&index_buffer_memory);

	uint16* ibo_data;
	result = vkMapMemory(graphics_state->device, index_buffer_memory, /*offset*/ 0, c_ibo_size, /*flags*/ 0, (void**)&ibo_data);
	assert(result == VK_SUCCESS);
	memcpy(ibo_data, indices, c_ibo_size);
	vkUnmapMemory(graphics_state->device, index_buffer_memory);

	constexpr uint32 c_num_shader_stages = 2;
	VkPipelineShaderStageCreateInfo shader_stage_info[c_num_shader_stages];
	shader_stage_info[0] = {};
	shader_stage_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_info[0].pNext = nullptr;
	shader_stage_info[0].flags = 0;
	shader_stage_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_info[0].module = create_shader_module(graphics_state->device, "shaders/shader.vert.spv", temp_allocator);
	shader_stage_info[0].pName = "main";
	shader_stage_info[0].pSpecializationInfo = nullptr;
	shader_stage_info[1] = {};
	shader_stage_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_info[1].pNext = nullptr;
	shader_stage_info[1].flags = 0;
	shader_stage_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_info[1].module = create_shader_module(graphics_state->device, "shaders/shader.frag.spv", temp_allocator);
	shader_stage_info[1].pName = "main";
	shader_stage_info[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription vertex_input_binding = {};
	vertex_input_binding.binding = 0;
	vertex_input_binding.stride = sizeof(float32) * c_float32_count_per_vertex;
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertex_input_attribute = {};
	vertex_input_attribute.location = 0;
	vertex_input_attribute.binding = 0;
	vertex_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute.offset = 0;

	VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
	vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_info.pNext = nullptr;
	vertex_input_state_info.flags = 0;
	vertex_input_state_info.vertexBindingDescriptionCount = 1;
	vertex_input_state_info.pVertexBindingDescriptions = &vertex_input_binding;
	vertex_input_state_info.vertexAttributeDescriptionCount = 1;
	vertex_input_state_info.pVertexAttributeDescriptions = &vertex_input_attribute;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
	input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_info.pNext = nullptr;
	input_assembly_state_info.flags = 0;
	input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_info.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float32)swapchain_info.imageExtent.width;
	viewport.height = (float32)swapchain_info.imageExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissors = {};
	scissors.offset.x = 0;
	scissors.offset.y = 0;
	scissors.extent = swapchain_info.imageExtent;

	VkPipelineViewportStateCreateInfo viewport_state_info = {};
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.pNext = nullptr;
	viewport_state_info.flags = 0;
	viewport_state_info.viewportCount = 1;
	viewport_state_info.pViewports = &viewport;
	viewport_state_info.scissorCount = 1;
	viewport_state_info.pScissors = &scissors;

	VkPipelineRasterizationStateCreateInfo rasterisation_state_info = {};
	rasterisation_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterisation_state_info.pNext = nullptr;
	rasterisation_state_info.flags = 0;
	rasterisation_state_info.depthClampEnable = false; // todo(jbr) is depth clamping good?
	rasterisation_state_info.rasterizerDiscardEnable = false;
	rasterisation_state_info.polygonMode = VK_POLYGON_MODE_LINE;
	rasterisation_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterisation_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterisation_state_info.depthBiasEnable = false;
	rasterisation_state_info.depthBiasConstantFactor = 0.0f;
	rasterisation_state_info.depthBiasClamp = 0.0f;
	rasterisation_state_info.depthBiasSlopeFactor = 0.0f;
	rasterisation_state_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
	multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_info.pNext = nullptr;
	multisample_state_info.flags = 0;
	multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_info.sampleShadingEnable = false;
	multisample_state_info.minSampleShading = 0.0f;
	multisample_state_info.pSampleMask = nullptr;
	multisample_state_info.alphaToCoverageEnable = false;
	multisample_state_info.alphaToOneEnable = false;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
	depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state_info.pNext = nullptr;
	depth_stencil_state_info.flags = 0;
	depth_stencil_state_info.depthTestEnable = true;
	depth_stencil_state_info.depthWriteEnable = true;
	depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state_info.depthBoundsTestEnable = false;
	depth_stencil_state_info.stencilTestEnable = false;
	depth_stencil_state_info.front = {};
	depth_stencil_state_info.back = {};
	depth_stencil_state_info.minDepthBounds = 0.0f;
	depth_stencil_state_info.maxDepthBounds = 0.0f;

	VkPipelineColorBlendAttachmentState colour_blend_attachment = {};
	colour_blend_attachment.blendEnable = false;
	colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colour_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	VkPipelineColorBlendStateCreateInfo colour_blend_state_info = {};
	colour_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_info.pNext = nullptr;
	colour_blend_state_info.flags = 0;
	colour_blend_state_info.logicOpEnable = false;
	colour_blend_state_info.logicOp = VK_LOGIC_OP_NO_OP;
	colour_blend_state_info.attachmentCount = 1;
	colour_blend_state_info.pAttachments = &colour_blend_attachment;
	colour_blend_state_info.blendConstants[0] = 1.0f;
	colour_blend_state_info.blendConstants[1] = 1.0f;
	colour_blend_state_info.blendConstants[2] = 1.0f;
	colour_blend_state_info.blendConstants[3] = 1.0f;

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext = nullptr;
	pipeline_info.flags = 0;
	pipeline_info.stageCount = c_num_shader_stages;
	pipeline_info.pStages = shader_stage_info;
	pipeline_info.pVertexInputState = &vertex_input_state_info;
	pipeline_info.pInputAssemblyState = &input_assembly_state_info;
	pipeline_info.pTessellationState = nullptr;
	pipeline_info.pViewportState = &viewport_state_info;
	pipeline_info.pRasterizationState = &rasterisation_state_info;
	pipeline_info.pMultisampleState = &multisample_state_info;
	pipeline_info.pDepthStencilState = &depth_stencil_state_info;
	pipeline_info.pColorBlendState = &colour_blend_state_info;
	pipeline_info.pDynamicState = nullptr;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = nullptr;
	pipeline_info.basePipelineIndex = 0;

	VkPipeline pipeline;
	result = vkCreateGraphicsPipelines(graphics_state->device, /*pipeline_cache*/ nullptr, /*pipeline_count*/ 1, &pipeline_info, /*allocator*/ nullptr, &pipeline);
	assert(result == VK_SUCCESS);

	matrix_4x4_projection(
		&graphics_state->projection_matrix,
		/*fov_y*/ 60.0f * c_deg_to_rad,
		/*aspect_ratio*/ swapchain_info.imageExtent.width / (float32)swapchain_info.imageExtent.height,
		/*near_plane*/ 0.1f,
		/*far_plane*/ 1000.0f);

	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.pNext = nullptr;
	command_pool_info.flags = 0;
	command_pool_info.queueFamilyIndex = graphics_queue_family_index;

	VkCommandPool command_pool;
	result = vkCreateCommandPool(graphics_state->device, &command_pool_info, /*allocator*/ nullptr, &command_pool);
	assert(result == VK_SUCCESS);

	VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
	command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.pNext = nullptr;
	command_buffer_alloc_info.commandPool = command_pool;
	command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount = swapchain_image_count;

	graphics_state->command_buffers = (VkCommandBuffer*)linear_allocator_alloc(allocator, sizeof(VkCommandBuffer) * swapchain_image_count);
	result = vkAllocateCommandBuffers(graphics_state->device, &command_buffer_alloc_info, graphics_state->command_buffers);
	assert(result == VK_SUCCESS);

	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = nullptr;
	command_buffer_begin_info.flags = 0;
	command_buffer_begin_info.pInheritanceInfo = nullptr;
	
	VkClearValue clear_values[2];
	clear_values[0] = {};
	clear_values[0].color.float32[0] = 0.0f;
	clear_values[0].color.float32[1] = 0.0f;
	clear_values[0].color.float32[2] = 0.0f;
	clear_values[0].color.float32[3] = 0.0f;
	clear_values[1] = {};
	clear_values[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.renderPass = render_pass;
	render_pass_begin_info.renderArea = scissors;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_values;

	for (uint32 command_buffer_i = 0; command_buffer_i < swapchain_image_count; ++command_buffer_i)
	{
		VkCommandBuffer command_buffer = graphics_state->command_buffers[command_buffer_i];

		result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
		assert(result == VK_SUCCESS);

		render_pass_begin_info.framebuffer = framebuffers[command_buffer_i];
		vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		for (int32 ubo_i = 0; ubo_i < graphics_state->ubo_count; ++ubo_i)
		{
			UBO* ubo = &graphics_state->ubos[ubo_i];

			vkCmdBindDescriptorSets(
				command_buffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				pipeline_layout, 
				/*first_set*/ 0, 
				/*set_count*/ 1, 
				&ubo->descriptor_set, 
				/*dynamic_offset_count*/ 0, 
				/*dynamic_offsets*/ nullptr);

			int32 global_instance_i = 0; // used for push constant which increases across models
			for (int32 model_i = 0; model_i < ubo->model_count; ++model_i)
			{
				UBO::Model_Info* model_info = &ubo->models[model_i];

				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(command_buffer, /*first_binding*/ 0, /*binding_count*/ 1, &model_info->vertex_buffer, &offset);

				vkCmdBindIndexBuffer(command_buffer, model_info->index_buffer, /*offset*/ 0, VK_INDEX_TYPE_UINT16);

				for (int32 instance_i = 0; instance_i < model_info->instance_count; ++instance_i)
				{
					vkCmdPushConstants(
						command_buffer,
						pipeline_layout,
						VK_SHADER_STAGE_VERTEX_BIT,
						/*offset*/ 0,
						sizeof(int32),
						&global_instance_i);

					vkCmdDrawIndexed(
						command_buffer, 
						model_info->index_count, 
						/*instance_count*/ 1, 
						/*first_index*/ 0, 
						/*vertex_offset*/ 0, 
						/*first_instance*/ 0);
					
					++global_instance_i;
				}
			}
			
		}

		vkCmdEndRenderPass(command_buffer);

		result = vkEndCommandBuffer(command_buffer);
		assert(result == VK_SUCCESS);
	}

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = nullptr;
	fence_info.flags = 0;

	vkCreateFence(graphics_state->device, &fence_info, /*allocator*/ nullptr, &graphics_state->fence);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext = nullptr;
	semaphore_info.flags = 0;

	vkCreateSemaphore(graphics_state->device, &semaphore_info, /*allocator*/ nullptr, &graphics_state->semaphore);
}

void graphics_draw(Graphics_State* graphics_state, Matrix_4x4* view_matrix)
{
	Matrix_4x4 view_projection_matrix;
	matrix_4x4_mul(&view_projection_matrix, &graphics_state->projection_matrix, view_matrix);

	UBO* ubos_end = &graphics_state->ubos[graphics_state->ubo_count];
	for (UBO* ubo = graphics_state->ubos; ubo != ubos_end; ++ubo)
	{
		Matrix_4x4* dst_transforms;
		VkResult result = vkMapMemory(
			graphics_state->device, 
			graphics_state->ubo_device_memory, 
			ubo->device_memory_offset, 
			ubo->transform_count * c_bytes_per_transform, 
			/*flags*/ 0, 
			(void**)&dst_transforms);
		assert(result == VK_SUCCESS);

		Matrix_4x4* src_transform = ubo->world_transforms;
		Matrix_4x4* dst_transform = dst_transforms;
		Matrix_4x4* dst_transforms_end = &dst_transforms[ubo->transform_count];
		for (; dst_transform != dst_transforms_end; ++src_transform, ++dst_transform)
		{
			matrix_4x4_mul(dst_transform, &view_projection_matrix, src_transform);
		}

		vkUnmapMemory(graphics_state->device, graphics_state->ubo_device_memory);
	}

	uint32 image_index;
	VkResult result = vkAcquireNextImageKHR(graphics_state->device, graphics_state->swapchain, /*timeout*/ (uint64)-1, graphics_state->semaphore, /*fence*/ nullptr, &image_index);
	assert(result == VK_SUCCESS);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &graphics_state->semaphore;
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &graphics_state->command_buffers[image_index];
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	result = vkQueueSubmit(graphics_state->graphics_queue, /*submit_count*/ 1, &submit_info, graphics_state->fence);
	assert(result == VK_SUCCESS);

	result = vkWaitForFences(graphics_state->device, /*fence_count*/ 1, &graphics_state->fence, /*wait_all*/ true, /*timeout*/ (uint64)-1);
	assert(result == VK_SUCCESS);

	result = vkResetFences(graphics_state->device, /*fence_count*/ 1, &graphics_state->fence);
	assert(result == VK_SUCCESS);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 0;
	present_info.pWaitSemaphores = nullptr;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &graphics_state->swapchain;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(graphics_state->present_queue, &present_info);
	assert(result == VK_SUCCESS);
}