#include "AccelerationEngine.hpp"
#include "ImplementationContext.hpp"
#include "ImplementationLogger.hpp"
#include "ImplementionManagedTypes.hpp"
#include "MemoryAllocator.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <cassert>
#ifdef _WIN32
#include <Windows.h>
#endif

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
		engineInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &engineInfo;

		std::vector<const char*> extensions;
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (DebugEnable) {
			_Logger = new HA::Logger(DebugMode == AccelerationEngineDebuggingOptions::WEB_SERVER);
			if (CheckInstanceSupport("VK_LAYER_KHRONOS_validation")) {
				extensions.push_back("VK_LAYER_KHRONOS_validation");
				instanceCreateInfo.pNext = &debugCreateInfo;
				debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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
		delete ImplementationContext->_CommandThread;
		if (ImplementationContext->Allocator)
			vmaDestroyAllocator(ImplementationContext->Allocator);
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
			case VK_API_VERSION_1_3:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_3;
				break;
			default:
				properties[i].SupportedVersion = AccelerationEngineVersion::VK_1_3;
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

	bool AccelerationEngine::UseDevice(const HardwareDevice& device)
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
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &ImplementationContext->Properties);

		vkGetDeviceQueue(ImplementationContext->Device, index, 0, &ImplementationContext->Queue);
		VmaAllocatorCreateInfo vcreateInfo{};
		vcreateInfo.physicalDevice = ImplementationContext->PhysicalDevice;
		vcreateInfo.device = ImplementationContext->Device;
		vcreateInfo.pAllocationCallbacks = ImplementationContext->AllocationCallbacks;
		vcreateInfo.instance = ImplementationContext->Instance;
		vcreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		vmaCreateAllocator(&vcreateInfo, &ImplementationContext->Allocator);

		ImplementationContext->_CommandThread = new CommandThread(ImplementationContext->Device, ImplementationContext->Queue, index, ImplementationContext->AllocationCallbacks);

		return true;
	}

	GPGPUBuffer* AccelerationEngine::Malloc(GPGPUMemoryType memoryType, uint64_t size)
	{
		return new GPGPUBuffer(ImplementationContext, memoryType, size);
	}

	void AccelerationEngine::FreeBuffer(GPGPUBuffer* buffer)
	{
		assert(buffer && "GPGPUBuffer cannot be null.");
		delete buffer;
	}

	void AccelerationEngine::CommitMemory()
	{
		ImplementationContext->_CommandThread->Execute();
	}

	bool AccelerationEngine::CheckVulkanSupport()
	{
#ifdef _WIN32
		HMODULE libraryTest = LoadLibraryA("vulkan-1.dll");
		if (libraryTest) {
			FreeLibrary(libraryTest);
			return true;
		}
		return false;
#else
#error "Not Supported"
#endif
		return true;
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
		HA::Logger* logger = (HA::Logger*)pUserData;
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
