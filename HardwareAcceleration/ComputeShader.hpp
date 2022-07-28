#pragma once
#include <cstdint>

namespace HA {

	class AccelerationEngine;

	class ComputeShader {

	public:
		ComputeShader(AccelerationEngine* engine, void* sourceCode, uint32_t length);
		ComputeShader(AccelerationEngine* engine, const char* path);
		~ComputeShader();
	public:
		/// <summary>
		/// VkShaderModule
		/// </summary>
		void* ComputeModule;

	private:
		void Load(AccelerationEngine* engine, void* sourceCode, uint32_t length);

	};

}
