//
// Tomato OS
// [Internal] 内存管理器
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#pragma once
#include <stdint.h>

struct _EFI_MEMORY_DESCRIPTOR;
typedef _EFI_MEMORY_DESCRIPTOR EFI_MEMORY_DESCRIPTOR;

namespace Tomato {
	namespace OS {

		class PhysicalMemory
		{
		public:
			void Initialize(const EFI_MEMORY_DESCRIPTOR* memoryDescriptor, size_t descriptorSize, size_t descriptorCount);

			void* AllocatePages(size_t pageCount);
			void FreePages(void* pageAddr, size_t pageCount);
		private:
			// 获取内存大小
			void AcquireMemorySize(const EFI_MEMORY_DESCRIPTOR* memoryDescriptor, size_t descriptorSize, size_t descriptorCount);
			// 初始化使用情况控制块
			void InitializeUsageControlBlock(const EFI_MEMORY_DESCRIPTOR* memoryDescriptor, size_t descriptorSize, size_t descriptorCount);
			void FlushUsageControlBlock();
		private:
			typedef uint64_t usageControlBlock_t;

			size_t physicalMemorySize;		// 物理内存大小
			usageControlBlock_t* usageControlBlock;	// 使用量控制块
			size_t usageControlBlockBits;
			size_t usageControlBlockSize;
			size_t pageUsed, pageAvaliable;
		};

		class MemoryManager
		{
		public:
			MemoryManager() = default;

			void Initialize(const EFI_MEMORY_DESCRIPTOR* memoryDescriptor, size_t descriptorSize, size_t descriptorCount);
			// 分配页
			void* AllocatePages(size_t pageCount);
			void FreePages(void* pageAddr, size_t pageCount);
		private:
			PhysicalMemory physicalMemory;
		};
	}
}