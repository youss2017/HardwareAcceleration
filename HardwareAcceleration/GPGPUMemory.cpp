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

HA::GPBuffer::GPBuffer(const ImplementationContext* Context, const GPGPUMemoryType memoryType, const uint64_t size)
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

HA::GPBuffer::~GPBuffer()
{
	if (MappedMemory)
		UnmapBuffer();
	vmaDestroyBuffer(Context->Allocator, Buffer->Buffer, Buffer->Allocation);
	delete Buffer;
}

HA::GPBuffer* HA::GPBuffer::Clone(HA::GPGPUMemoryType memoryType)
{
	return new GPBuffer(Context, memoryType, Size);
}

HA::GPBuffer* HA::GPBuffer::Copy(HA::GPGPUMemoryType memoryType)
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

void* HA::GPBuffer::MapBuffer() {
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

void HA::GPBuffer::UnmapBuffer()
{
	if (MemoryType == GPGPUMemoryType::Static) {
		delete[] MappedMemory;
	}
	else {
		vmaUnmapMemory(Context->Allocator, Buffer->Allocation);
	}
	MappedMemory = nullptr;
}

void HA::GPBuffer::Write(void* Data, uint64_t offset, uint64_t size)
{
	assert((offset + size) <= Size);
	if (MemoryType != GPGPUMemoryType::Static) {
		bool unmap = !MappedMemory;
		if (!MappedMemory)
			MapBuffer();
		if (!MappedMemory) {
			throw std::runtime_error("Error Mapping Buffer.");
		}
		memcpy(((char*)MappedMemory) + offset, Data, size);
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, offset, size);
		if (unmap)
			UnmapBuffer();
	}
	else {
		//assert(0);
		auto cmd = Context->_CommandThread->GenCmd();
		auto fence = Context->_CommandThread->GenFence();
		VkBufferCopy copy{};
		copy.dstOffset = offset;
		copy.size = size;
		auto stage = new GPBuffer(Context, GPGPUMemoryType::Host, size);
		stage->Write(Data, 0, size);
		vkCmdCopyBuffer(cmd, stage->Buffer->Buffer, Buffer->Buffer, 1, &copy);
		Context->_CommandThread->Execute(cmd, fence, true);
		Context->_CommandThread->ReleaseFence(fence);
		delete stage;
	}
}

void HA::GPBuffer::WriteAsync(void* Data, uint64_t offset, uint64_t size) {
	if (MemoryType != GPGPUMemoryType::Static) {
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
		auto stage = new GPBuffer(Context, GPGPUMemoryType::Host, size);
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

void HA::GPBuffer::StoreToDisk(const char* FileName)
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

void HA::GPBuffer::SyncWrite(uint64_t offset, uint64_t size)
{
	size = size == 0 ? Size : size;
	if (MemoryType != GPGPUMemoryType::Static) {
		vmaFlushAllocation(Context->Allocator, Buffer->Allocation, offset, size);
	}
	else {
		Write(((char*)MappedMemory) + offset, offset, size);
	}
}

void HA::GPBuffer::SyncRead()
{
	assert(MappedMemory && "Buffer must be mapped to perform SyncRead()");
	if (MemoryType != GPGPUMemoryType::Static) {
		vmaInvalidateAllocation(Context->Allocator, Buffer->Allocation, 0, Size);
	}
	else {
		auto cmd = Context->_CommandThread->GenCmd();
		auto fence = Context->_CommandThread->GenFence();
		auto stage = new GPBuffer(Context, GPGPUMemoryType::Host, Size);
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

void HA::GPBuffer::Sync(uint64_t offset, uint64_t size)
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

#pragma endregion

#pragma region GPU Image
#define IMAGE_ASPECT (VK_IMAGE_ASPECT_COLOR_BIT)

void HA::GPImage::Write(uint32_t SizeInBytes, uint8_t* PixelData)
{
	GPBuffer* stage = new GPBuffer(Context, GPGPUMemoryType::Host, SizeInBytes);
	stage->Write(PixelData, 0, SizeInBytes);
	Write(stage);
	delete stage;
}

void HA::GPImage::Write(GPBuffer* buffer)
{
	auto cmd = Context->_CommandThread->GenCmd();
	auto fence = Context->_CommandThread->GenFence();
	TransitionImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = IMAGE_ASPECT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;
	region.imageExtent = Size;
	vkCmdCopyBufferToImage(cmd, buffer->Buffer->Buffer, Image->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	TransitionImage(cmd, VK_IMAGE_LAYOUT_GENERAL);
	Context->_CommandThread->Execute(cmd, fence, true);
	Context->_CommandThread->ReleaseFence(fence);
}

void HA::GPImage::ReadBack(GPBuffer** OutBuffer, GPGPUMemoryType MemoryType)
{
	assert(OutBuffer);
	GPBuffer* buffer = new GPBuffer(Context, MemoryType, BufferRowLength * Size.height);
	if (!buffer) {
		if (Context->Logger) {
			Context->Logger->Print("Cannot readback image because buffer allocation failed.");
		}
	}
	auto cmd = Context->_CommandThread->GenCmd();
	auto fence = Context->_CommandThread->GenFence();
	TransitionImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = IMAGE_ASPECT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;
	region.imageExtent = Size;
	vkCmdCopyImageToBuffer(cmd, Image->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->Buffer->Buffer, 1, &region);
	TransitionImage(cmd, ReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL);
	Context->_CommandThread->Execute(cmd, fence, true);
	Context->_CommandThread->ReleaseFence(fence);
	*OutBuffer = buffer;
}

HA::GPImage* HA::GPImage::Clone(GPGPUMemoryType memoryType)
{
	auto image = new HA::GPImage(Context, Format, ImageType, Size, BufferRowLength, Mipcount, memoryType);
	if (!image) {
		if (Context->Logger) {
			Context->Logger->Print("Could not create image.");
		}
		throw std::runtime_error("Could not create image.");
	}
	return image;
}

HA::GPImage* HA::GPImage::Copy(GPGPUMemoryType memoryType)
{
	auto image = Clone(memoryType);
	GPBuffer* imageData;
	ReadBack(&imageData, GPGPUMemoryType::Host);
	if (!imageData) {
		if (Context->Logger) {
			Context->Logger->Print("Could not allocate buffer.");
		}
		throw std::runtime_error("Could not allocate buffer.");
	}
	image->Write(imageData);
	delete imageData;
	return image;
}

void HA::GPImage::OptimizeShaderAccess(bool ReadOnly)
{
	auto cmd = Context->_CommandThread->GenCmd();
	auto fence = Context->_CommandThread->GenFence();
	TransitionImage(cmd, ReadOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL);
	Context->_CommandThread->Execute(cmd, fence, true);
	Context->_CommandThread->ReleaseFence(fence);
	this->ReadOnly = ReadOnly;
}

void HA::GPImage::TransitionImage(VkCommandBuffer cmd, VkImageLayout layout)
{
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = CurrentLayout;
	barrier.newLayout = layout;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Image->Image;
	barrier.subresourceRange.aspectMask = IMAGE_ASPECT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	CurrentLayout = layout;
}

HA::GPImage::GPImage(
	const ImplementationContext* Context,
	const VkFormat format,
	const VkImageType type,
	const VkExtent3D size,
	const uint32_t RowLengthInBytes,
	const int Mipcount,
	const GPGPUMemoryType memoryType)
	: Context(Context), MemoryType(memoryType), Format(format), ImageType(type),
	Size(size), BufferRowLength(RowLengthInBytes), Mipcount(Mipcount), CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
	ReadOnly(false)
{
	auto image = new ImplementationManagedImage;
	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = type;
	createInfo.format = format;
	createInfo.extent = size;
	createInfo.mipLevels = Mipcount;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage =
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_STORAGE_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VmaAllocationCreateInfo allocCreateInfo{};
	auto result = vmaCreateImage(Context->Allocator, &createInfo, &allocCreateInfo,
		&image->Image, &image->Allocation, &image->AllocationInfo);
	if (result != VK_SUCCESS) {
		delete image;
		throw std::runtime_error("VMA: Encountered error creating image.");
	}
	Image = image;
}

HA::GPImage::~GPImage()
{
	vmaDestroyImage(Context->Allocator, Image->Image, Image->Allocation);
	delete Image;
}

#pragma endregion
