#pragma once
#include <cstdint>

namespace HA {

	struct ImplementationManagedBuffer;
	struct ImplementationContext;

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
		/// Creates another buffer with similar properties. Also allows you to change memory type
		/// </summary>
		/// <returns></returns>
		GPGPUBuffer* Clone(HA::GPGPUMemoryType memoryType);

		/// <summary>
		/// Same as Clone() but also copies content. Also allows you to change memory type
		/// </summary>
		/// <returns></returns>
		GPGPUBuffer* Copy(HA::GPGPUMemoryType memoryType);

		/// <summary>
		/// If your only writing use memcpy() because C++ '=' operator may perform read operation,
		/// depending on memory type read operation may cause a memory sync from GPU to CPU which may be
		/// not neccessary for write only operations.
		/// [Warning] Writes or Reads may be performed before SyncWrite or SyncRead
		/// </summary>
		/// <returns>CPU Mapped pointer</returns>
		void* MapBuffer();

		/// <summary>
		/// Frees the mapped memory
		/// </summary>
		void UnmapBuffer();

		/// <summary>
		/// Writes data directly 
		/// </summary>
		/// <param name="Data"></param>
		/// <param name="offset"></param>
		/// <param name="size"></param>
		void Write(void* Data, uint64_t offset, uint64_t size);
		void WriteAsync(void* Data, uint64_t offset, uint64_t size);

		/// <summary>
		/// Reads Buffer into memory and then stores to disk as raw .bin file.
		/// This function uses the C Runtime FILE* API
		/// </summary>
		/// <param name="FileName">Path</param>
		void StoreToDisk(const char* FileName);

		/// <summary>
		/// Guarantees GPU writes are available to the CPU. Must have memory mapped.
		/// <param name="offset">The starting location of MapMemory()</param>
		/// <param name="size">The range to update the buffer on the GPU. Size of 0 = Buffer Size</param>
		/// </summary>
		void SyncWrite(uint32_t offset, uint32_t size);

		/// <summary>
		/// Guarantees CPU writes are available to the GPU. Must have memory mapped.
		/// </summary>
		void SyncRead();

		/// <summary>
		/// SyncRead() and SyncWrite(). Must have memory mapped.
		/// <param name="offset">The starting location of MapMemory()</param>
		/// <param name="size">The range to update the buffer on the GPU. Size of 0 = Buffer Size</param>
		/// </summary>
		void Sync(uint32_t offset, uint32_t size);

	public:
		const GPGPUMemoryType MemoryType;
		const uint64_t Size;
		const ImplementationManagedBuffer* Buffer;

	private:
		void* MappedMemory;
		const ImplementationContext* Context;

	private:
		friend class AccelerationEngine;
		GPGPUBuffer(const ImplementationContext* Context, const GPGPUMemoryType memoryType, const uint64_t size);
		~GPGPUBuffer();
	};

}
