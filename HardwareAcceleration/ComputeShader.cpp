#define _CRT_SECURE_NO_WARNINGS
#include "ComputeShader.hpp"
#include <stdio.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <shaderc/shaderc.hpp>

HA::ComputeShader::ComputeShader(AccelerationEngine* engine, void* sourceCode, uint32_t length)
{
	this->Load(engine, sourceCode, length);
}

HA::ComputeShader::ComputeShader(AccelerationEngine* engine, const char* path)
{
	FILE* io = fopen(path, "r");
	if (!io)
		throw std::runtime_error("HA::ComputeShader Could not open file to read source code.");
	fseek(io, 0, SEEK_END);
	uint32_t length = ftell(io);
	fseek(io, 0, SEEK_SET);
	char* buffer = new char[length];
	length = (uint32_t)fread(buffer, 1, length, io);
	this->Load(engine, buffer, length);
	delete[] buffer;
}

HA::ComputeShader::~ComputeShader()
{
}

void HA::ComputeShader::Load(AccelerationEngine* engine, void* sourceCode, uint32_t length)
{
	// 1) Compile Shader
	shaderc::Compiler comp;
	comp.CompileGlslToSpv((char*)sourceCode, length, shaderc_shader_kind::shaderc_compute_shader, "main.comp");

	// 2) Create Shader Module
	//VkShaderModuleCreateInfo createInfo;
}
