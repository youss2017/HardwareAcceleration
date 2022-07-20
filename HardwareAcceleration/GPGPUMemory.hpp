#pragma once
#include <cstdint>

namespace HA {

	struct ImplementationManagedBuffer;

	enum class GPGPUMemoryType {
		/// <summary>
		/// Stored directly on GPU Memory with no direct access for read/write operations
		/// CPU Read/Writes are slowest here because intermediate buffers have to be used.
		/// </summary>
		Static,
		/// <summary>
		/// Note: if not available, Host memory type will be used.
		/// Stored in both CPU RAM and GPU VRAM good for occusionally reads/write
		/// Not as fast and efficient as Static but faster than Host
		/// </summary>
		Stream,
		/// <summary>
		/// Stored on CPU RAM and is uploaded every time the GPU accesses the resource.
		/// </summary>
		Host
	};

	class GPGPUBuffer {
	public:
		/// <summary>
		/// </summary>
		/// <param name="memoryType"></param>
		/// <param name="size">Size is in bytes</param>
		GPGPUBuffer(const GPGPUBuffer& copy) = delete;
		GPGPUBuffer(const GPGPUBuffer&& move) = delete;

		/// <summary>
		/// Creates another buffer with similar properties
		/// </summary>
		/// <returns></returns>
		GPGPUBuffer Clone();

		/// <summary>
		/// Same as Clone() but also copies content.
		/// </summary>
		/// <returns></returns>
		GPGPUBuffer Copy();

		/// <summary>
		/// If your only writing use memcpy() because C++ '=' operator may perform read operation,
		/// depending on memory type read operation may cause a memory sync from GPU to CPU which may be
		/// not neccessary for write only operations.
		/// </summary>
		/// <returns>CPU Mapped pointer</returns>
		void* MapBuffer();

		/// <summary>
		/// Guarantees CPU writes are available to the GPU
		/// </summary>
		void FlushWrites();

		/// <summary>
		/// Guarantees GPU writes are available to the CPU
		/// </summary>
		void SyncWrites();

	public:
		const GPGPUMemoryType MemoryType;
		const uint64_t Size;
		const ImplementationManagedBuffer* Buffer;

	private:
		friend class AccelerationEngine;
		GPGPUBuffer(GPGPUMemoryType memoryType, uint64_t size, ImplementationManagedBuffer* buffer) 
			: MemoryType(memoryType), Size(size), Buffer(buffer) {}
	};

}
