#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace HA {

	struct ImplementationManagedBuffer {
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;
	};

}
