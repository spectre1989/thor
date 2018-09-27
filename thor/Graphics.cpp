#include "Graphics.h"

#include "Memory.h"
#include <vulkan/vulkan.h>
#include <cstdio>



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
		int x = 1;
		x *= 2;
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
			gpu_memory_properties->memoryTypes[i].propertyFlags & required_properties)
		{
			return i;
		}
	}

	return (uint32)-1;
}

void graphics_init(Graphics_State* graphics_state, HINSTANCE instance_handle, HWND window_handle, uint32 width, uint32 height, Linear_Allocator* temp_allocator)
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
	for (uint32 i = 0; i < gpu_count; ++i)
	{
		VkPhysicalDeviceProperties gpu_properties;
		vkGetPhysicalDeviceProperties(gpus[i], &gpu_properties);

		if (gpu_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			graphics_state->gpu = gpus[i];
			break;
		}
	}
	if (!graphics_state->gpu)
	{
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
	device_info.pEnabledFeatures = nullptr;

	result = vkCreateDevice(graphics_state->gpu, &device_info, /*allocator*/ nullptr, &graphics_state->device);
	assert(result == VK_SUCCESS);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics_state->gpu, surface, &surface_capabilities);
	assert(result == VK_SUCCESS);

	uint32 surface_format_count;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_state->gpu, surface, &surface_format_count, /*surface_formats*/ nullptr);
	assert(result == VK_SUCCESS);

	VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)linear_allocator_alloc(temp_allocator, sizeof(VkSurfaceFormatKHR) * surface_format_count);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_state->gpu, surface, &surface_format_count, surface_formats);
	assert(result == VK_SUCCESS);

	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = nullptr;
	swapchain_info.flags = 0;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = surface_capabilities.minImageCount;
	VkFormat swapchain_image_format = surface_formats[0].format != VK_FORMAT_UNDEFINED ? surface_formats[0].format : VK_FORMAT_B8G8R8A8_UNORM; // todo(jbr) pick best image format/colourspace
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

	VkSwapchainKHR swapchain;
	result = vkCreateSwapchainKHR(graphics_state->device, &swapchain_info, /*allocator*/ nullptr, &swapchain);
	assert(result == VK_SUCCESS);

	uint32 swapchain_image_count;
	result = vkGetSwapchainImagesKHR(graphics_state->device, swapchain, &swapchain_image_count, /*swapchain_images*/ nullptr);
	assert(result == VK_SUCCESS);

	VkImage* swapchain_images = (VkImage*)linear_allocator_alloc(temp_allocator, sizeof(VkImage) * swapchain_image_count);
	result = vkGetSwapchainImagesKHR(graphics_state->device, swapchain, &swapchain_image_count, swapchain_images);
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
	depth_buffer_info.format = VK_FORMAT_D32_SFLOAT;
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

	VkMemoryRequirements memory_requirements = {};
	vkGetImageMemoryRequirements(graphics_state->device, depth_buffer, &memory_requirements);

	VkMemoryAllocateInfo mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = nullptr;
	mem_alloc_info.allocationSize = memory_requirements.size;
	mem_alloc_info.memoryTypeIndex = get_memory_type_index(&gpu_memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

	constexpr uint32 c_matrix_count = 256;
	constexpr uint32 c_ubo_size = c_matrix_count * 4 * 4 * sizeof(float32);

	VkBufferCreateInfo uniform_buffer_info = {};
	uniform_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	uniform_buffer_info.pNext = nullptr;
	uniform_buffer_info.flags = 0;
	uniform_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uniform_buffer_info.size = c_ubo_size;
	uniform_buffer_info.queueFamilyIndexCount = 0;
	uniform_buffer_info.pQueueFamilyIndices = nullptr;
	uniform_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	VkBuffer uniform_buffer;
	result = vkCreateBuffer(graphics_state->device, &uniform_buffer_info, /*allocator*/ nullptr, &uniform_buffer);
	assert(result == VK_SUCCESS);

	mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = nullptr;
	mem_alloc_info.allocationSize = memory_requirements.size;
	mem_alloc_info.memoryTypeIndex = get_memory_type_index(&gpu_memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	assert(mem_alloc_info.memoryTypeIndex != (uint32)-1);

	VkDeviceMemory uniform_buffer_memory;
	result = vkAllocateMemory(graphics_state->device, &mem_alloc_info, /*allocator*/ nullptr, &uniform_buffer_memory);
	assert(result == VK_SUCCESS);

	float* matrix_data;
	result = vkMapMemory(graphics_state->device, uniform_buffer_memory, /*offset*/ 0, c_ubo_size, /*flags*/ 0, (void**)&matrix_data);
	assert(result == VK_SUCCESS);

	for (int32 matrix_i = 0; matrix_i < c_matrix_count; ++matrix_i)
	{
		for (int32 component_i = 0; component_i < 16; ++component_i)
		{
			matrix_data[component_i] = 0.0f;
		}
	}

	vkUnmapMemory(graphics_state->device, uniform_buffer_memory);

	result = vkBindBufferMemory(graphics_state->device, uniform_buffer, uniform_buffer_memory, /*offset*/ 0);
	assert(result == VK_SUCCESS);

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

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext = nullptr;
	pipeline_layout_info.flags = 0;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

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
	buffer_info.range = memory_requirements.size;

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
}