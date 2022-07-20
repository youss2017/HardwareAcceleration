#pragma once
#include <vector>
#include <cstdint>
#include "GPGPUMemory.hpp"

namespace HA {

	struct ImplementationContext;
	class Logger;

	/// <summary>
	/// Tells the engine which Vulkan version to use
	/// </summary>
	enum class AccelerationEngineVersion {
		VK_1_0,
		VK_1_1,
		VK_1_2
	};

	enum AccelerationEngineDebuggingOptions {
		WEB_SERVER,
		CONSOLE
	};

	/// <summary>
	/// Describes the properties of the graphics card.
	/// </summary>
	struct HardwareDevice {
		uint64_t Id;
		char Name[256];
		uint64_t VRAMSize;
		uint64_t SystemSharedMemorySize;
		bool IsDedicated;
		AccelerationEngineVersion SupportedVersion;
	};

	class AccelerationEngine {

	public:
		AccelerationEngine(
			bool DebugEnable = true, 
			AccelerationEngineDebuggingOptions DebugMode = AccelerationEngineDebuggingOptions::WEB_SERVER);
		~AccelerationEngine();
		AccelerationEngine(const AccelerationEngine& copy) = delete;
		AccelerationEngine(const AccelerationEngine&& move) = delete;

		/// <summary>
		/// Enumerates properties of all available devices.
		/// </summary>
		/// <returns>List of detected of graphics cards</returns>
		std::vector<HardwareDevice> EnumerateAvailableDevices();

		/// <summary>
		/// Performs neccessary setup and initalization for the device,
		/// to allow the engine to be able to use the device. This must
		/// be called before any other API function (other than EnumerateAvailableDevices)
		/// </summary>
		/// <param name="device"></param>
		/// <returns>True if the device is supported, false then use a different device.</returns>
		bool EstablishDevice(const HardwareDevice& device);

		/// <summary>
		/// Creates a new buffer with specified requirements
		/// </summary>
		/// <param name="memoryType"></param>
		/// <param name="size"></param>
		/// <returns>If memory allocation is succesful a pointer to the buffer is returned otherwise nullptr is returned</returns>
		GPGPUBuffer* AllocateBuffer(GPGPUMemoryType memoryType, uint64_t size);
		/// <summary>
		/// Frees memory used by buffer
		/// </summary>
		/// <param name="buffer"></param>
		void FreeBuffer(GPGPUBuffer* buffer);

	public:
		/// <summary>
		/// Determines whether Vulkan validation layers are enabled.
		/// </summary>
		const bool DebugEnable;
		/// <summary>
		/// Determines output target of validation info/warning/error
		/// </summary>
		const AccelerationEngineDebuggingOptions DebugMode;
		/// <summary>
		/// Internal Use Only by the API. The user should not require this object.
		/// </summary>
		ImplementationContext* ImplementationContext;

	private:
		/// <summary>
		/// Internal Use Only by the API. Manages info/warning/error logging.
		/// </summary>
		Logger* _Logger;

	};

}
