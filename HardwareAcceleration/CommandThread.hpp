#pragma once
#include <vulkan/vulkan_core.h>
#include <functional>
#include <vector>

namespace HA {

	class CommandThread {
	public:
		CommandThread(VkDevice device, VkQueue queue, uint32_t queueFamilyIndex, VkAllocationCallbacks* Callbacks);
		~CommandThread();
		CommandThread(const CommandThread& copy) = delete;
		CommandThread(const CommandThread&& move) = delete;

		VkFence GenFence();
		void ReleaseFence(VkFence fence);

		VkCommandBuffer GetCmd();
		VkCommandBuffer GenCmd();

		void Execute();
		void Execute(VkCommandBuffer cmd, VkFence fence, bool FreeCmd);

		void AddPostExectute(std::function<void()>& postExec);

	private:
		VkDevice Device;
		VkQueue Queue;
		VkAllocationCallbacks* AllocationCallbacks;
		VkCommandPool Pool;
		VkCommandBuffer SharedCmd;
		VkFence Fence;
		std::vector<std::function<void()>> PostFunctions;
	};

}