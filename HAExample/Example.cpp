#include <iostream>
#include <AccelerationEngine.hpp>
#pragma comment(lib, "HardwareAcceleration.lib")
#pragma comment(lib, "vulkan-1.lib")
using namespace std;

int main(int argc, char** argv) {

	HA::AccelerationEngine* engine = new HA::AccelerationEngine();
	
	auto devices = engine->EnumerateAvailableDevices();
	engine->EstablishDevice(devices[0]);

	for (auto& device : devices) {
		printf("%s --- Video RAM %llu MB; System Ram %llu MB\n", device.Name,
			device.VRAMSize / (1024 * 1024), device.SystemSharedMemorySize / (1024 * 1024));
	}

	HA::GPGPUBuffer* buffer = engine->AllocateBuffer(HA::GPGPUMemoryType::Stream, 100 * 1024 * 1024);
	engine->FreeBuffer(buffer);
	delete engine;
	int exitCode;
	cout << "Press any key to exit" << endl;
	cin >> exitCode;
	return exitCode;
}