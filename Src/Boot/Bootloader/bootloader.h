//
// Tomato OS
// Bootloader
// (c) 2015 SunnyCase
// 创建日期：2015-4-19
#pragma once
#include "kernel.h"
#include "mmu.hpp"

#define MPrint(fmt, ...) Print((const CHAR16*)fmt, __VA_ARGS__)

#define EXIT_IF_NOT_SUCCESS(status, imageHandle, fmt, ...) if(EFI_ERROR((status))){\
MPrint(fmt, __VA_ARGS__); \
gBS->Exit(imageHandle, status, 0, nullptr); }

static UINTN KernelPoolSize = 1024 * 1024; // 1 MB

inline UINTN AlignSize(UINTN size, UINTN align)
{
	UINTN newSize = size - size % align;
	if (newSize < size) newSize += align;
	return newSize;
}

class KernelFile
{
public:
	KernelFile(EFI_HANDLE imageHandle)
		:imageHandle(imageHandle){}
	KernelFile(KernelFile&& file);
	~KernelFile();

	void LoadKernelFile();
	bool ValidateKernel();
	KernelEntryPoint GetEntryPoint();

	UINT8* GetKernelFileBuffer() const { return kernelFileBuffer; }
private:
	KernelFile& operator=(KernelFile&) = delete;
	KernelFile(KernelFile&) = delete;
private:
	EFI_HANDLE imageHandle;
	UINT8* kernelFileBuffer;
};

class Bootloader
{
public:
	Bootloader(EFI_HANDLE imageHandle)
		:imageHandle(imageHandle){}

	void Boot();
private:
	void PrintMemoryMap();
	void PreparePaging();
	void MappingFrameBuffer(Tomato::OS::PDPTable& pdpTable);
	// 启用分页
	void EnablePaging();
	KernelFile LoadKernel();
	void PrepareKernel(KernelFile& file);
	void RunKernel(KernelEntryPoint entryPoint);
	void InitializeKernelEntryParams();
	void PrepareVirtualMemoryMapping();
	void SetGraphicsMode();
private:
	EFI_HANDLE imageHandle;
	EFI_PHYSICAL_ADDRESS kernelMemBuffer;
	EFI_IMAGE_SECTION_HEADER* sectionStart;
	UINTN sectionCount;
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	KernelEntryParams params;
	Tomato::OS::PML4Table* pml4Table;
};

