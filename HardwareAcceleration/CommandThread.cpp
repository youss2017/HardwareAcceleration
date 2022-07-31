#include "CommandThread.hpp"

HA::CommandThread::CommandThread(VkDevice device, VkQueue queue, uint32_t queueFamilyIndex, VkAllocationCallbacks* Callbacks)
	:Device(device), Queue(queue), AllocationCallbacks(Callbacks)
{
	Fence = GenFence();
	VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndex;
	vkCreateCommandPool(device, &createInfo, AllocationCallbacks, &Pool);
	SharedCmd = GenCmd();
}

HA::CommandThread::~CommandThread()
{
	ReleaseFence(Fence);
	vkDestroyCommandPool(Device, Pool, AllocationCallbacks);
}

VkFence HA::CommandThread::GenFence()
{
	VkFence fence;
	VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence(Device, &createInfo, AllocationCallbacks, &fence);
	return fence;
}

void HA::CommandThread::ReleaseFence(VkFence fence)
{
	vkDestroyFence(Device, fence, AllocationCallbacks);
}

VkCommandBuffer HA::CommandThread::GetCmd()
{
	return SharedCmd;
}

VkCommandBuffer HA::CommandThread::GenCmd()
{
	VkCommandBuffer cmd;
	VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = Pool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(Device, &allocInfo, &cmd);
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);
	return cmd;
}

void HA::CommandThread::Execute()
{
	Execute(SharedCmd, Fence, false);
	for (const auto& postExec : PostFunctions)
		postExec();
	}

void HA::CommandThread::Execute(VkCommandBuffer cmd, VkFence fence, bool FreeCmd)
{
	vkEndCommandBuffer(cmd);
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(Queue, 1, &submitInfo, fence);
	vkWaitForFences(Device, 1, &fence, true, UINT64_MAX);
	if (FreeCmd)
		vkFreeCommandBuffers(Device, Pool, 1, &cmd);
	else {
		vkResetCommandBuffer(SharedCmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &beginInfo);
	}

}

void HA::CommandThread::AddPostExectute(std::function<void()>& postExec)
{
	PostFunctions.push_back(postExec);
}
