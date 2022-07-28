#include <iostream>
#include <string>
#include <AccelerationEngine.hpp>
#pragma comment(lib, "HardwareAcceleration.lib")
#pragma comment(lib, "vulkan-1.lib")
using namespace std;

int main(int argc, char** argv) {

	if (!HA::AccelerationEngine::CheckVulkanSupport()) {
		cout << "The current system does not support vulkan." << endl;
		return 0;
	}
	cout << "Vulkan Detected." << endl;

	HA::AccelerationEngine* engine = new HA::AccelerationEngine(true, HA::AccelerationEngineDebuggingOptions::CONSOLE);
	
	auto devices = engine->EnumerateAvailableDevices();
	engine->UseDevice(devices[0]);

	for (auto& device : devices) {
		printf("%s --- Video RAM %llu MB; System Ram %llu MB\n", device.Name,
			device.VRAMSize / (1024 * 1024), device.SystemSharedMemorySize / (1024 * 1024));
	}

	HA::GPGPUBuffer* buffer = engine->Malloc(HA::GPGPUMemoryType::Static, 10 * 1024 * 1024);
	std::string dob = "2008/01/01";
	buffer->Write((void*)dob.data(), 0, dob.size());
	//engine->CommitMemory();
	auto copy = buffer->Copy(HA::GPGPUMemoryType::Host);
	engine->FreeBuffer(buffer);

	auto mappedData = copy->MapBuffer();
	copy->SyncRead();
	printf("['%s']\n", mappedData);

	engine->FreeBuffer(copy);
	delete engine;
	return 0;
}