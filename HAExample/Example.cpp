#include <iostream>
#include <string>
#include <AccelerationEngine.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#pragma comment(lib, "HardwareAcceleration.lib")
#pragma comment(lib, "vulkan-1.lib")
using namespace std;

/// The purpose of this file is to show
/// how to use the api and be used as a test unit.

int main(int argc, char** argv) {

	if (!HA::AccelerationEngine::CheckVulkanSupport()) {
		cout << "The current system does not support vulkan." << endl;
		return 0;
	}
	cout << "Vulkan Detected." << endl;

	HA::AccelerationEngine* engine = new HA::AccelerationEngine(true, HA::AccelerationEngineDebuggingOptions::CONSOLE);

	auto devices = engine->EnumerateAvailableDevices();
	auto device = HA::HardwareDevice::GetDefault(devices);
	engine->UseDevice(device);

	printf("%s [%llu] --- Video RAM %llu MB; System Ram %llu MB\n", device.Name, device.Id,
		device.VRAMSize / (1024 * 1024), device.SystemSharedMemorySize / (1024 * 1024));

	HA::GPBuffer* buffer = new HA::GPBuffer(engine->ImplementationContext, HA::GPGPUMemoryType::Static, 10 * 1024 * 1024);
	std::string dob = "2008/01/01";
	buffer->WriteAsync((void*)dob.data(), 0, dob.size());
	engine->CommitMemory();
	auto copy = buffer->Copy(HA::GPGPUMemoryType::Host);
	delete buffer;

	auto mappedData = (char*)copy->MapBuffer();
	copy->SyncRead();
	printf("['%s']\n", mappedData);
	delete copy;

	using namespace HA;
	VkExtent3D extent{ 512, 512, 1 };
	GPImage* image = new GPImage(engine->ImplementationContext, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TYPE_2D, extent, 512 * sizeof(int32_t), 1, HA::GPGPUMemoryType::Static);
	{
		int x, y, c;
		auto pepper = stbi_load("pepper.bmp", &x, &y, &c, 4);
		image->Write(x * y * sizeof(int32_t), pepper);
		stbi_image_free(pepper);
	}

	GPImage* imageCopy = image->Copy(GPGPUMemoryType::Host);
	delete image;

	{
		GPBuffer* imageReadback;
		imageCopy->ReadBack(&imageReadback, GPGPUMemoryType::Host);
		auto* memory = imageReadback->MapBuffer();
		stbi_write_bmp("pepper_readback.bmp", 512, 512, 4, memory);
		delete imageReadback;
	}

	delete imageCopy;
	delete engine;
	return 0;
}