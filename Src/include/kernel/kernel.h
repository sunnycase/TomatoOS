//
// Tomato OS
// [Internal] 内核定义
// (c) 2015 SunnyCase
// 创建日期：2015-4-18
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>

	typedef struct
	{
		UINT8* AcpiTable;
		SMBIOS_TABLE_ENTRY_POINT* SmbiosTable;
		EFI_RUNTIME_SERVICES* RT;
		EFI_MEMORY_DESCRIPTOR* MemoryDescriptor;
		UINTN MemoryDescriptorEntryCount;
		UINTN MemoryDescriptorSize;
		UINTN FrameWidth;
		UINTN FrameHeight;
		EFI_PHYSICAL_ADDRESS FrameBufferBase;
		UINT32 FrameBufferSize;

	} KernelEntryParams;

	typedef void(__cdecl *KernelEntryPoint)(KernelEntryParams);

#define KernelPoolType (EFI_MEMORY_TYPE)0x80000001
#define KernelPagingDataType (EFI_MEMORY_TYPE)0x80000002
#define KernelImageBase 0x140000000ui64
#define KernelSize (uint64_t)(2 * 1024 * 1024)
#define PAGE_SIZE 4096

#ifdef __cplusplus
}
#endif