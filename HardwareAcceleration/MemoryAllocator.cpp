#if 0
#include "MemoryAllocator.hpp"
#include "ImplementationContext.hpp"
#include "ImplementionManagedTypes.hpp"
#include <optional>
#include <cassert>

using namespace HA;
static VkMemoryPropertyFlags GetPreferredFlags(GPGPUMemoryType memoryType);

HA::MemoryAllocator::MemoryAllocator(ImplementationContext* context)
	: Context(context)
{}

HA::MemoryAllocator::~MemoryAllocator()
{
	for (auto& [Id, Heap] : Allocations) {
		vkFreeMemory(Context->Device, Heap.Memory, Context->AllocationCallbacks);
	}
}

HA::GPGPUBuffer* HA::MemoryAllocator::CreateBuffer(GPGPUMemoryType memoryType, const VkBufferCreateInfo& createInfo)
{
	VkBuffer buffer;
	if (vkCreateBuffer(Context->Device, &createInfo, Context->AllocationCallbacks, &buffer) != VK_SUCCESS) {
		return nullptr;
	}
	VkMemoryRequirements memoryRequirement;
	vkGetBufferMemoryRequirements(Context->Device, buffer, &memoryRequirement);
	uint32_t HeapId, SubAllocId;
	if (!Search((VkOpaque)buffer, memoryType, true, &HeapId, &SubAllocId, memoryRequirement)) {
		vkDestroyBuffer(Context->Device, buffer, Context->AllocationCallbacks);
		return nullptr;
	}
	HA::ImplementationManagedBuffer* managedBuffer = new HA::ImplementationManagedBuffer;
	managedBuffer->Buffer = buffer;
	managedBuffer->AllocationId = SubAllocId;
	managedBuffer->HeapId = HeapId;
	return new HA::GPGPUBuffer(memoryType, createInfo.size, managedBuffer);
}

void HA::MemoryAllocator::FreeBuffer(GPGPUBuffer* buffer)
{
	assert(buffer && "GPGPUBuffer is nullptr");
	const ImplementationManagedBuffer* managed = buffer->Buffer;
	AllocationAccessKey.lock();
	
	Allocations[managed->HeapId].Suballocations[managed->AllocationId].Used = false;
	
	AllocationAccessKey.unlock();
	vkDestroyBuffer(Context->Device, managed->Buffer, Context->AllocationCallbacks);
	delete buffer->Buffer;
	delete buffer;
}

static inline std::optional<uint32_t> findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	return std::optional<int>();
}

bool HA::MemoryAllocator::CreateHeap(bool Locked, uint32_t typeFilter, GPGPUMemoryType memoryType, int size, bool preferred) {
	// Attempt allocating through every heap before returning error.
	VkMemoryPropertyFlags flags = preferred ? GetPreferredFlags(memoryType) : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (int i = 0; i < Context->Properties.memoryHeapCount; i++) {
		VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
		allocateInfo.allocationSize = size;
		std::optional<int> memoryTypeIndex = findMemoryType(Context->PhysicalDevice, typeFilter, GetPreferredFlags(memoryType));
		if (!memoryTypeIndex.has_value())
			continue;
		allocateInfo.memoryTypeIndex = memoryTypeIndex.value();
		VkDeviceMemory Heap;
		VkResult result = vkAllocateMemory(Context->Device, &allocateInfo, Context->AllocationCallbacks, &Heap);
		if (result == VK_SUCCESS) {
			if (!Locked)
				AllocationAccessKey.lock();
			HeapAllocation allocation{};
			allocation.Memory = Heap;
			allocation.Size = size;
			allocation.MemoryProperties = flags;
			allocation.Id = Allocations.size();
			Allocations.insert({ Allocations.size(), allocation });
			if (!Locked)
				AllocationAccessKey.unlock();
			return true;
		}
	}
	if (preferred)
		return CreateHeap(Locked, typeFilter, memoryType, size, false);
	return false;
}

bool HA::MemoryAllocator::Search(VkOpaque Handle, 
	GPGPUMemoryType memoryType, 
	bool isBuffer, 
	uint32_t* OutHeapId, 
	uint32_t* OutSubAllocId, 
	const VkMemoryRequirements& memoryRequirement)
{
	AllocationAccessKey.lock();
	for (auto& [Id, Allocation] : Allocations) {
		if (Allocation.Size < memoryRequirement.size)
			continue;
		uint64_t Offset = 0;
		if (Allocation.Suballocations.size() > 0) {
			auto& lastAllocation = Allocation.Suballocations.end()->second;
			Offset = lastAllocation.Offset + lastAllocation.Size;
		}
		Offset -= Offset % memoryRequirement.alignment;
		Offset += memoryRequirement.alignment;
		if (Allocation.Size - Offset < memoryRequirement.size)
			continue;
		if (isBuffer)
			vkBindBufferMemory(Context->Device, (VkBuffer)Handle, Allocation.Memory, Offset);
		else
			vkBindImageMemory(Context->Device, (VkImage)Handle, Allocation.Memory, Offset);
		HeapSubAllocation suballocation{ Allocation.Suballocations.size() };
		suballocation.Id = Allocation.Suballocations.size();
		suballocation.Offset = Offset;
		suballocation.ParentAllocation = &Allocation;
		suballocation.Size = memoryRequirement.size;
		suballocation.Used = true;
		Allocation.Suballocations.insert({ suballocation.Id, suballocation });
		*OutHeapId = Allocation.Id;
		*OutSubAllocId = suballocation.Id;
		AllocationAccessKey.unlock();
		return true;
	}
	if (!CreateHeap(true, memoryRequirement.memoryTypeBits, memoryType, 256 * 1024 * 1024, true)) {
		AllocationAccessKey.unlock();
		return false;
	}
	AllocationAccessKey.unlock();
	return Search(Handle, memoryType, isBuffer, OutHeapId, OutSubAllocId, memoryRequirement);
}

static VkMemoryPropertyFlags GetPreferredFlags(GPGPUMemoryType memoryType) {
	switch (memoryType) {
	case GPGPUMemoryType::Static:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case GPGPUMemoryType::Stream:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	default:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
}
#endif
