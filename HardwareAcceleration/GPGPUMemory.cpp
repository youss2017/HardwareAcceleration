#define _CRT_SECURE_NO_WARNINGS
#include "GPGPUMemory.hpp"
#include "ImplementionManagedTypes.hpp"
#include "ImplementationContext.hpp"
#include <vma/vk_mem_alloc.h>
#include <cassert>
#include <stdexcept>

#pragma region GPU Buffer


static VkMemoryPropertyFlags GetPreferredFlags(HA::GPGPUMemoryType memoryType) {
	switch (memoryType) {
	case HA::GPGPUMemoryType::Static:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case HA::GPGPUMemoryType::Stream:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	default:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
}

HA::GPGPUBuffer::GPGPUBuffer(const ImplementationContext* Context, const GPGPUMemoryType memoryType, const uint64_t size)
	: Context(Context), MemoryType(memoryType), Size(size), MappedMemory(nullptr) {

	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	VmaAllocationCreateInfo acreateInfo{};
	acreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	acreateInfo.preferredFlags = GetPreferredFlags(memoryType);
	acreateInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT |
		(memoryType == GPGPUMemoryType::Static ? 0 : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(Context->Allocator, &createInfo, &acreateInfo, &buffer, &allocation, &allocationInfo);
	ImplementationManagedBuffer* managedBuffer = new HA::ImplementationManagedBuffer;
	managedBuffer->Buffer = buffer;
	managedBuffer->Allocation = allocation;
	managedBuffer->AllocationInfo = allocationInfo;
	Buffer = managedBuffer;
}

HA::GPGPUBuffer::~GPGPUBuffer()
{
	if (MappedMemory)
		UnmapBuffer();
	vmaDestroyBuffer(Context->Allocator, Buffer->Buffer, Buffer->Allocation);
	delete Buffer;
}

HA::GPGPUBuffer* HA::GPGPUBuffer::Clone(HA::GPGPUMemoryType memoryType)
{
	return new GPGPUBuffer(Context, memoryType, Size);
}

HA::GPGPUBuffer* HA::GPGPUBuffer::Copy(HA::GPGPUMemoryType memoryType)
{
	auto copy = this->Clone(memoryType);
	bool unmap = !MappedMemory;
	MapBuffer();
	SyncRead();
	copy->Write(MappedMemory, 0, Size);
	if (unmap)
		UnmapBuffer();
	return copy;
}

void* HA::GPGPUBuffer::MapBuffer() {
	if (MappedMemory)
		return MappedMemory;
	if (MemoryType == GPGPUMemoryType::Static) {
		if (!MappedMemory)
			MappedMemory = new char[Size];
	}
	else {
		vmaMapMemory(Context->Allocator, Buffer->Allocation, &MappedMemory);
	}
	return MappedMemory;
}

void HA::GPGPUBuffer::UnmapBuffer()
{
	if (MemoryType == GPGPUMemoryType::Static) {
		delete[] MappedMemory;
	}
	else {
		vmaUnmapMemory(Context->Allocator, Buffer->Allocation);
	}
	MappedMemory = nullptr;
}

void HA::GPGPUBuffer::Write(void* Data, uint64_t offset, uint64_t size)
{
	assert((offset + size) <= Size);
	if (MemoryType != GPGPUMemoryType::Static) {
		if (!MappedMemory)
			MapBuffer();
		if (!MappedMemory) {
			throw std::runtime_error("Error Mapping Buffer.");
		}
		memcpy(((char*)MappedMemory) + offset, Data, size);
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, offset, size);
	}
	else {
		//assert(0);
		auto cmd = Context->_CommandThread->GenCmd();
		auto fence = Context->_CommandThread->GenFence();
		VkBufferCopy copy{};
		copy.dstOffset = offset;
		copy.size = size;
		auto stage = new GPGPUBuffer(Context, GPGPUMemoryType::Host, size);
		stage->Write(Data, 0, size);
		vkCmdCopyBuffer(cmd, stage->Buffer->Buffer, Buffer->Buffer, 1, &copy);
		Context->_CommandThread->Execute(cmd, fence, true);
		Context->_CommandThread->ReleaseFence(fence);
		delete stage;
	}
}

void HA::GPGPUBuffer::WriteAsync(void* Data, uint64_t offset, uint64_t size) {
	if (MemoryType == GPGPUMemoryType::Static) {
		bool unmap = !MappedMemory;
		MapBuffer();
		assert(MappedMemory);
		memcpy((char*)MappedMemory + offset, Data, size);
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, offset, size);
		if (unmap)
			UnmapBuffer();
	}
	else {
		auto cmd = Context->_CommandThread->GetCmd();
		auto stage = new GPGPUBuffer(Context, GPGPUMemoryType::Host, size);
		char* stagemem = (char*)stage->MapBuffer();
		memcpy(stagemem, Data, size);
		VkBufferCopy region{};
		region.dstOffset = offset;
		region.size = size;
		vkCmdCopyBuffer(cmd, stage->Buffer->Buffer, Buffer->Buffer, 1, &region);
		std::function<void()> func = [stage]() {
			delete stage;
		};
		Context->_CommandThread->AddPostExectute(func);
	}
}

void HA::GPGPUBuffer::StoreToDisk(const char* FileName)
{
	bool unmapFlag = !MappedMemory;
	if (MappedMemory)
		MapBuffer();
	SyncRead();
	if (!MappedMemory) {
		return;
	}
	FILE* io = fopen(FileName, "wb");
	assert(io);
	fwrite(MappedMemory, 1, Size, io);
	fclose(io);
	if (unmapFlag)
		UnmapBuffer();

}

void HA::GPGPUBuffer::SyncWrite(uint32_t offset, uint32_t size)
{
	size = size == 0 ? Size : size;
	if (MemoryType != GPGPUMemoryType::Static) {
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, offset, size);
	}
	else {
		Write(((char*)MappedMemory) + offset, offset, size);
	}
}

void HA::GPGPUBuffer::SyncRead()
{
	assert(MappedMemory && "Buffer must be mapped to perform SyncRead()");
	if (MemoryType != GPGPUMemoryType::Static) {
		vmaInvalidateAllocation(Context->Allocator, Buffer->Allocation, 0, Size);
	}
	else {
		auto cmd = Context->_CommandThread->GenCmd();
		auto fence = Context->_CommandThread->GenFence();
		auto stage = new GPGPUBuffer(Context, GPGPUMemoryType::Host, Size);
		VkBufferCopy region{};
		region.size = Size;
		vkCmdCopyBuffer(cmd, Buffer->Buffer, stage->Buffer->Buffer, 1, &region);
		Context->_CommandThread->Execute(cmd, fence, true);
		Context->_CommandThread->ReleaseFence(fence);
		auto ptr = stage->MapBuffer();
		memcpy(MappedMemory, ptr, Size);
		stage->UnmapBuffer();
		delete stage;
	}
}

void HA::GPGPUBuffer::Sync(uint32_t offset, uint32_t size)
{
	if (MemoryType != GPGPUMemoryType::Static) {
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, 0, Size);
		vmaInvalidateAllocation(Context->Allocator, Buffer->Allocation, 0, Size);
	}
	else {
		SyncWrite(offset, size);
		SyncRead();
	}
}

#pragma endregion GPU Buffer