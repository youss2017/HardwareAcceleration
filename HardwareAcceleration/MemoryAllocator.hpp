#pragma once
#if 0
#include <vulkan/vulkan_core.h>
#include <list>
#include <mutex>
#include <map>
#include "GPGPUMemory.hpp"

// Nonstandard Handle Type
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkOpaque)

namespace HA {

	struct ImplementationContext;

	struct HeapSubAllocation {
		uint32_t Id;
		uint64_t Offset;
		uint64_t Size;
		bool Used;
		// Pointer to HeapAllocation
		void* ParentAllocation;
	};

	struct HeapAllocation {
		uint64_t Id;
		VkDeviceMemory Memory;
		uint64_t Size;
		VkMemoryPropertyFlags MemoryProperties;
		std::map<uint32_t, HeapSubAllocation> Suballocations;
	};

	class MemoryAllocator {

	public:
		MemoryAllocator(ImplementationContext* context);
		MemoryAllocator(const MemoryAllocator& other) = delete;
		MemoryAllocator(const MemoryAllocator&& other) = delete;
		~MemoryAllocator();

		GPGPUBuffer* CreateBuffer(GPGPUMemoryType memoryType, const VkBufferCreateInfo& createInfo);
		void FreeBuffer(GPGPUBuffer* buffer);

	private:
		bool CreateHeap(bool Locked, uint32_t typeFilter, GPGPUMemoryType memoryType, int size, bool preferred);
		bool Search(VkOpaque Handle,
			GPGPUMemoryType memoryType,
			bool isBuffer,
			uint32_t* OutHeapId,
			uint32_t* OutSubAllocId,
			const VkMemoryRequirements& memoryRequirement);

	public:
		const ImplementationContext* Context;

	protected:
		std::map<uint32_t, HeapAllocation> Allocations;
		std::mutex AllocationAccessKey;

	};

}
#endif
