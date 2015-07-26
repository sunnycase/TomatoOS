//
// Tomato OS
// 内存管理器
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#include "stdafx.h"
#include "MemoryManager.h"
#include "InternelStates.h"
#include "../../../include/common/math.h"
#include "../../../include/kernel/kernel.h"
#include <Uefi.h>
#include <Library/UefiLib.h>

using namespace Tomato::OS;

inline bool IsMemoryEntryAvailable(const EFI_MEMORY_DESCRIPTOR * memoryDescriptor)
{
	auto type = memoryDescriptor->Type;
	if (type == EfiConventionalMemory ||
		type == EfiLoaderCode ||
		type == EfiLoaderData ||
		type == EfiBootServicesCode ||
		type == EfiBootServicesData)
		return true;
	return false;
}

static wchar_t *OsLoaderMemoryTypeDesc[EfiMaxMemoryType] = {
	L"reserved  ",
	L"LoaderCode",
	L"LoaderData",
	L"BS_code   ",
	L"BS_data   ",
	L"RT_code   ",
	L"RT_data   ",
	L"available ",
	L"Unusable  ",
	L"ACPI_recl ",
	L"ACPI_NVS  ",
	L"MemMapIO  ",
	L"MemPortIO ",
	L"PAL_code  "
};

// 每位代表 1 页 4KB
enum : size_t
{
	EachBitManageSize = PAGE_SIZE,
	EachBits = 64,
	EachManage = EachBitManageSize * EachBits
};

void MemoryManager::Initialize(const EFI_MEMORY_DESCRIPTOR * memoryDescriptor, size_t descriptorSize, size_t descriptorCount)
{
	bootVideo.PutString(L"  Type        Start Address     End Address       Virtual Address      \r\n");
	bootVideo.PutString(L"  ==========  ================  ================  ================\r\n");
	const EFI_MEMORY_DESCRIPTOR* MemoryMapEntry = memoryDescriptor;
	for (UINTN i = 0; i < descriptorCount; i++)
	{
		auto physicalEnd = MemoryMapEntry->PhysicalStart + (MemoryMapEntry->NumberOfPages << EFI_PAGE_SHIFT) - 1;
		bootVideo.PutFormat(L"  %s  %x     %x     %x\r\n",
			MemoryMapEntry->Type == KernelPoolType ? L"KernelPool" : MemoryMapEntry->Type == KernelPagingDataType ? L"KernelPage" : OsLoaderMemoryTypeDesc[MemoryMapEntry->Type],
			MemoryMapEntry->PhysicalStart,
			physicalEnd,
			MemoryMapEntry->VirtualStart);
		MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, descriptorSize);
	}
	physicalMemory.Initialize(memoryDescriptor, descriptorSize, descriptorCount);
}

void * MemoryManager::AllocatePages(size_t pageCount)
{
	return physicalMemory.AllocatePages(pageCount);
}

void MemoryManager::FreePages(void * pageAddr, size_t pageCount)
{
	return physicalMemory.FreePages(pageAddr, pageCount);
}

void PhysicalMemory::Initialize(const EFI_MEMORY_DESCRIPTOR* memoryDescriptor, size_t descriptorSize, size_t descriptorCount)
{
	AcquireMemorySize(memoryDescriptor, descriptorSize, descriptorCount);
	InitializeUsageControlBlock(memoryDescriptor, descriptorSize, descriptorCount);

	bootVideo.PutFormat(L"Page Used: %d, Page Available: %d\r\n", pageUsed, pageAvaliable);
}

void * PhysicalMemory::AllocatePages(size_t pageCount)
{
	if (pageCount)
	{
		auto controlBlock = usageControlBlock;
		size_t fullfilled = 0, fullfillIndex = 0, fullfillBitIndex = 0;
		// 寻找满足条件的连续页
		for (size_t bit = 0, i = 0; bit < usageControlBlockBits;i++)
		{
			auto value = *controlBlock++;
			for (size_t j = 0; j < 64 && bit < usageControlBlockBits; bit++, j++)
			{
				if (!TestBit(value, j))
				{
					if (fullfilled == 0)
					{
						fullfillIndex = i;
						fullfillBitIndex = j;
					}
					// 满足条件
					if (++fullfilled == pageCount)
					{
						auto bitIndex = fullfillBitIndex;
						auto rest = fullfilled;
						// 标记使用
						for (size_t i = fullfillIndex; rest; i++)
						{
							auto& value = usageControlBlock[i];
							for (size_t j = bitIndex; j < 64 && rest; j++, rest--)
								value |= (1ui64 << j);
							bitIndex = 0;
						}
						pageUsed += fullfilled;
						pageAvaliable -= fullfilled;

						return (void*)(PAGE_SIZE *(fullfillIndex * 64 + fullfillBitIndex));
					}
				}
				else
					fullfilled = 0;
			}
		}
	}
	return nullptr;
}

void PhysicalMemory::FreePages(void* pageAddr, size_t pageCount)
{
	auto index = size_t(pageAddr) / EachManage;
	auto bitIndex = (size_t(pageAddr) % EachManage) / EachBitManageSize;
	auto rest = pageCount;
	// 标记使用
	for (size_t i = index; rest; i++)
	{
		auto& value = usageControlBlock[i];
		for (size_t j = bitIndex; j < 64 && rest; j++, rest--)
			value &= ~(1ui64 << j);
		bitIndex = 0;
	}
	pageUsed -= pageCount;
	pageAvaliable += pageCount;
}

void PhysicalMemory::AcquireMemorySize(const EFI_MEMORY_DESCRIPTOR * memoryDescriptor, size_t descriptorSize, size_t descriptorCount)
{
	size_t memoryAddrEnd = 0;
	const EFI_MEMORY_DESCRIPTOR* MemoryMapEntry = memoryDescriptor;
	for (UINTN i = 0; i < descriptorCount; i++)
	{
		auto physicalEnd = MemoryMapEntry->PhysicalStart + (MemoryMapEntry->NumberOfPages << EFI_PAGE_SHIFT) - 1;
		if (physicalEnd > memoryAddrEnd)
			memoryAddrEnd = physicalEnd;
		MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, descriptorSize);
	}
	physicalMemorySize = memoryAddrEnd + 1;
	bootVideo.PutFormat(L"Memory Size: %x\r\n", physicalMemorySize);
}

void PhysicalMemory::InitializeUsageControlBlock(const EFI_MEMORY_DESCRIPTOR * memoryDescriptor, size_t descriptorSize, size_t descriptorCount)
{
	usageControlBlockBits = physicalMemorySize / EachBitManageSize;
	usageControlBlockSize = CeilDiv(usageControlBlockBits, EachBits);
	// 控制块需要的页数
	const auto usageControlBlockPageSize = CeilDiv(usageControlBlockSize, PAGE_SIZE);
	// 寻找内存中可用的页并能存得下控制块
	const EFI_MEMORY_DESCRIPTOR* memoryMapEntry = memoryDescriptor,
		*usedByControlBlock = nullptr;
	for (UINTN i = 0; i < descriptorCount; i++)
	{
		// 这时候不能动 LoaderData，因为我们的栈空间可能还在里面
		if (memoryMapEntry->Type != EfiLoaderData && IsMemoryEntryAvailable(memoryMapEntry)
			&& memoryMapEntry->NumberOfPages >= usageControlBlockPageSize)
		{
			usedByControlBlock = memoryMapEntry;
			break;
		}
		memoryMapEntry = NEXT_MEMORY_DESCRIPTOR(memoryMapEntry, descriptorSize);
	}
	// 如果没找到则系统失败
	if (!usedByControlBlock)
		KeFastFail(L"Cannot initialize Memory Manager.\r\n");
	// 填充控制块
	usageControlBlock = (usageControlBlock_t*)usedByControlBlock->PhysicalStart;
	FlushUsageControlBlock();

	memoryMapEntry = memoryDescriptor;
	for (UINTN i = 0; i < descriptorCount; i++)
	{
		if (IsMemoryEntryAvailable(memoryMapEntry))
		{
			auto phyicalAddr = memoryMapEntry->PhysicalStart;
			const auto controlBlockIndex = phyicalAddr / EachManage;
			const auto controlBlockRest = phyicalAddr % EachManage;
			auto controlBlockBitIndex = controlBlockRest / EachBitManageSize;
			auto restBits = memoryMapEntry->NumberOfPages;
			pageAvaliable += restBits;

			auto controlBlock = usageControlBlock + controlBlockIndex;
			for (; restBits; controlBlock++)
			{
				for (size_t j = controlBlockBitIndex; j < 64 && restBits; j++, restBits--)
					*controlBlock &= ~(1ui64 << j);
				controlBlockBitIndex = 0;
			}
		}
		memoryMapEntry = NEXT_MEMORY_DESCRIPTOR(memoryMapEntry, descriptorSize);
	}
	// 标记控制块本身使用的内存
	auto phyicalAddr = (size_t)usageControlBlock;
	const auto controlBlockIndex = phyicalAddr / EachManage;
	const auto controlBlockRest = phyicalAddr % EachManage;
	auto controlBlockBitIndex = controlBlockRest / EachBitManageSize;
	auto restBits = usageControlBlockPageSize;
	pageAvaliable -= restBits;

	auto controlBlock = usageControlBlock + controlBlockIndex;
	for (; restBits; controlBlock++)
	{
		for (size_t i = controlBlockBitIndex; i < sizeof(usageControlBlock_t) * 8
			&& restBits; i++, restBits--)
			*controlBlock |= (1ui64 << i);
		controlBlockBitIndex = 0;
	}
	pageUsed = usageControlBlockBits - pageAvaliable;
}

void PhysicalMemory::FlushUsageControlBlock()
{
	const auto count = usageControlBlockSize;
	auto* controlBlock = usageControlBlock;
	for (size_t i = 0; i < count; i++)
		*controlBlock++ = UINT64_MAX;
	pageAvaliable = 0;
	pageUsed = usageControlBlockBits;
}