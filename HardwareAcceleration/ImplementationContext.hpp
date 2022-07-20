#pragma once
// This file is only for internal use by the api
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>

namespace HA {

	struct ImplementationContext {
		VkAllocationCallbacks* AllocationCallbacks;
		VkInstance Instance;
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VmaAllocator Allocator;
	};

	std::string GetStringFromResult(VkResult result);
	std::string GetDescriptionFromResult(VkResult result);

}
