#include "AccelerationEngine.hpp"
#include "ImplementationContext.hpp"
#include "ImplementationLogger.hpp"
#include "ImplementionManagedTypes.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <iostream>

namespace HA {

	static bool CheckInstanceSupport(const char* layerName);
	static VKAPI_ATTR VkBool32 VKAPI_CALL HA_VulkanValidation_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	AccelerationEngine::AccelerationEngine(bool DebugEnable, AccelerationEngineDebuggingOptions DebugMode)
		: DebugEnable(DebugEnable), DebugMode(DebugMode), _Logger(nullptr)
	{
		ImplementationContext = new HA::ImplementationContext();
		ImplementationContext->AllocationCallbacks = nullptr;

		VkApplicationInfo engineInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		engineInfo.pApplicationName = "HardwareAccelerationAPI";
		engineInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		engineInfo.engineVersion = engineInfo.applicationVersion;
		engineInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &engineInfo;

		std::vector<const char*> extensions;
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (DebugEnable) {
			_Logger = new Logger(DebugMode == AccelerationEngineDebuggingOptions::WEB_SERVER);
			if (CheckInstanceSupport("VK_LAYER_KHRONOS_validation")) {
				extensions.push_back("VK_LAYER_KHRONOS_validation");
				instanceCreateInfo.pNext = &debugCreateInfo;
				debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				debugCreateInfo.pfnUserCallback = HA_VulkanValidation_DebugCallback;
				debugCreateInfo.pUserData = _Logger;
			}
			else {
				std::cout << "Validation layers not supported." << std::endl;
			}
		}
		instanceCreateInfo.enabledLayerCount = (uint32_t)extensions.size();
		instanceCreateInfo.ppEnabledLayerNames = extensions.data();

		VkResult result = vkCreateInstance(&instanceCreateInfo, ImplementationContext->AllocationCallbacks, &ImplementationContext->Instance);
		if (result != VK_SUCCESS) {
			if (DebugEnable)
				_Logger->Print("Failed to create the vulkan instance. Could not initalize engine.", true, "red");
			throw std::runtime_error("Failed to create the vulkan instance. Could not initalize engine.");
		}

	}

	AccelerationEngine::~AccelerationEngine()
	{
		if (ImplementationContext->Device) {
			vkDestroyDevice(ImplementationContext->Device, ImplementationContext->AllocationCallbacks);
		}
		vkDestroyInstance(ImplementationContext->Instance, ImplementationContext->AllocationCallbacks);
		if (_Logger)
			delete _Logger;
		if (ImplementationContext->AllocationCallbacks)
			delete ImplementationContext->AllocationCallbacks;
		delete ImplementationContext;
	}

	std::vector<HardwareDevice> AccelerationEngine::EnumerateAvailableDevices()
	{
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(ImplementationContext->Instance, &count, nullptr);
		std::vector<VkPhysicalDevice> physicalDevices(count);
		vkEnumeratePhysicalDevices(ImplementationContext->Instance, &count, physicalDevices.data());

		std::vector<HardwareDevice> properties(count);

		int i = 0;
		for (auto physicalDevice : physicalDevices) {
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(physicalDevice, &prop);
			switch (prop.apiVersion) {
			case VK_API_VERSION_1_0:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_0;
				break;
			case VK_API_VERSION_1_1:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_1;
				break;
			case VK_API_VERSION_1_2:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_2;
				break;
			default:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_2;
			}
			memcpy(properties[i].Name, prop.deviceName, sizeof(prop.deviceName));
			properties[i].IsDedicated = !((prop.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) |
				(prop.deviceType & VK_PHYSICAL_DEVICE_TYPE_CPU));
			properties[i].Id = (unsigned long long)physicalDevice;
			VkPhysicalDeviceMemoryProperties memProp;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);
			properties[i].VRAMSize = 0;
			properties[i].SystemSharedMemorySize = 0;
			for (uint32_t j = 0; j < memProp.memoryHeapCount; j++) {
				if (memProp.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
					properties[i].VRAMSize += memProp.memoryHeaps[j].size;
				}
				else {
					properties[i].SystemSharedMemorySize += memProp.memoryHeaps[j].size;
				}
			}
			i++;
		}

		return properties;
	}

	bool AccelerationEngine::EstablishDevice(const HardwareDevice& device)
	{
		VkPhysicalDevice physicalDevice = (VkPhysicalDevice)device.Id;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		int index = 0x7fffffff;
		for (auto& queueFamily : queueFamilyProps) {
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				index = index ^ 0x7fffffff;
				break;
			}
			index++;
		}

		if (index & 0x7fffffff) {
			return false;
		}

		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, ImplementationContext->AllocationCallbacks, &ImplementationContext->Device);
		if (result != VK_SUCCESS) {
			return false;
		}
		ImplementationContext->PhysicalDevice = physicalDevice;
		return true;
	}

	GPGPUBuffer* AccelerationEngine::AllocateBuffer(GPGPUMemoryType memoryType, uint64_t size)
	{
		if (!ImplementationContext->Allocator) {
			VmaAllocatorCreateInfo createInfo{};
			createInfo.device = ImplementationContext->Device;
			createInfo.instance = ImplementationContext->Instance;
			createInfo.physicalDevice = ImplementationContext->PhysicalDevice;
			createInfo.pAllocationCallbacks = ImplementationContext->AllocationCallbacks;
			createInfo.vulkanApiVersion = VK_API_VERSION_1_1;
			if (!vmaCreateAllocator(&createInfo, &ImplementationContext->Allocator) == VK_SUCCESS) {
				if (DebugEnable) {
					_Logger->Print("Could not initalize Vulkan Memory Allocator (VMA).", true, "red");
				}
				return nullptr;
			}
		}
		VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		createInfo.size = size;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VmaAllocationCreateInfo vmaCreateInfo{};
		vmaCreateInfo.memoryTypeBits = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
		switch (memoryType) {
		case GPGPUMemoryType::Static:
			vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			break;
		case GPGPUMemoryType::Host:
			vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			break;
		default:
			vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		}
		ImplementationManagedBuffer* buffer = new ImplementationManagedBuffer();
		VkResult result = vmaCreateBuffer(ImplementationContext->Allocator, &createInfo, &vmaCreateInfo, &buffer->Buffer, &buffer->Allocation, &buffer->AllocationInfo);
		if (result != VK_SUCCESS) {
			if (DebugEnable) {
				_Logger->Print(GetStringFromResult(result).c_str(), true, "red");
				_Logger->Print(GetDescriptionFromResult(result).c_str(), true, "red");
			}
			return nullptr;
		}
		return new GPGPUBuffer(memoryType, size, buffer);
	}

	void AccelerationEngine::FreeBuffer(GPGPUBuffer* buffer)
	{
		vmaDestroyBuffer(ImplementationContext->Allocator, buffer->Buffer->Buffer, buffer->Buffer->Allocation);
		delete buffer->Buffer;
		delete buffer;
	}

	static bool CheckInstanceSupport(const char* layerName) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
		return true;

	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL HA_VulkanValidation_DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		Logger* logger = (Logger*)pUserData;
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			logger->Print(pCallbackData->pMessage, false, "blue");
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			logger->Print(pCallbackData->pMessage, false, "orange");
		}
		else if (!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
			logger->Print(pCallbackData->pMessage, true, "red");
		}
		else {
			logger->Print(pCallbackData->pMessage);
		}

		return VK_TRUE;
	}

}