//
// Tomato OS
// Bootloader 分页
// (c) 2015 SunnyCase
// 创建日期：2015-4-19
#include "stdafx.h"
#include "bootloader.h"

using namespace Tomato::OS;

static_assert(EFI_PAGE_SIZE == PTEntryManageSize, "EFI_PAGE_SIZE must be equal to PTEntryManageSize.");

PML4Table* AllocatePML4Table(EFI_HANDLE ImageHandle)
{
	// 512 项 PML4 Entry 映射 256 TB，每项 512 GB
	enum {
		PML4Size = sizeof(PML4Table),
		PML4PageCount = PML4Size / EFI_PAGE_SIZE
	};
	PML4Table* pml4Addr;
	EXIT_IF_NOT_SUCCESS(gBS->AllocatePages(AllocateAnyPages, KernelPagingDataType,
		PML4PageCount, (EFI_PHYSICAL_ADDRESS*)&pml4Addr), ImageHandle, L"Cannot allocate PML4 Table.\r\n");
	SetMem(pml4Addr, PML4Size, 0);
	return pml4Addr;
}

PDPTable* AllocatePDPTable(EFI_HANDLE ImageHandle)
{
	// 512 项 Page Directory Pointer Entry 映射 512 GB，每项 1 GB
	enum {
		PDPSize = sizeof(PDPTable),
		PDPPageCount = PDPSize / EFI_PAGE_SIZE
	};
	PDPTable* pdpAddr;
	EXIT_IF_NOT_SUCCESS(gBS->AllocatePages(AllocateAnyPages, KernelPagingDataType,
		PDPPageCount, (EFI_PHYSICAL_ADDRESS*)&pdpAddr), ImageHandle, L"Cannot allocate PDP Table.\r\n");
	SetMem(pdpAddr, PDPSize, 0);
	return pdpAddr;
}

PageDirectory* AllocatePageDirectory(EFI_HANDLE ImageHandle)
{
	// 512 项 Page Directory Entry 映射 1 GB，每项 2 MB
	enum {
		PDSize = sizeof(PageDirectory),
		PDPageCount = PDSize / EFI_PAGE_SIZE
	};
	PageDirectory* pdAddr;
	EXIT_IF_NOT_SUCCESS(gBS->AllocatePages(AllocateAnyPages, KernelPagingDataType,
		PDPageCount, (EFI_PHYSICAL_ADDRESS*)&pdAddr), ImageHandle, L"Cannot allocate Page Directory.\r\n");
	SetMem(pdAddr, PDSize, 0);
	return pdAddr;
}

PageTable* AllocatePageTable(EFI_HANDLE ImageHandle)
{
	// 512 项 Page Table Entry 映射 2 MB，每项 4 KB
	enum {
		PTSize = sizeof(PageTable),
		PTPageCount = PTSize / EFI_PAGE_SIZE
	};
	PageTable* ptAddr;
	EXIT_IF_NOT_SUCCESS(gBS->AllocatePages(AllocateAnyPages, KernelPagingDataType,
		PTPageCount, (EFI_PHYSICAL_ADDRESS*)&ptAddr), ImageHandle, L"Cannot allocate Page Table.\r\n");
	SetMem(ptAddr, PTSize, 0);
	return ptAddr;
}

// 映射前 1 GB
// 实际映射物理内存的 64 MB
void MappingLow1GB(EFI_HANDLE ImageHandle, PDPTable& pdpTable)
{
	enum : uint64_t {
		PML4EIndex = uint64_t(64 * 1024 * 1024) / PML4EntryManageSize,
		PML4ERest = uint64_t(64 * 1024 * 1024) % PML4EntryManageSize,
		PDPEIndex = PML4ERest / PDPEntryManageSize,
		PDPERest = PML4ERest % PDPEntryManageSize,
		PDEIndex = 0,
		PDEEndIndex = PDPERest / PDEntryManageSize
	};

	auto& pageDir = *AllocatePageDirectory(ImageHandle);
	auto& pageDirRef = pdpTable[PDPEIndex];
	pageDirRef.Present = TRUE;
	pageDirRef.ReadWrite = TRUE;
	pageDirRef.SetPageDirectoryAddress(pageDir);

	uint8_t* physicalAddr = 0;
	for (size_t i = 0; i < PDEEndIndex; i++)
	{
		auto& pageDirEntry = pageDir[i];
		auto& pageTable = *AllocatePageTable(ImageHandle);

		for (auto& ptEntry : pageTable)
		{
			ptEntry.Present = TRUE;
			ptEntry.ReadWrite = TRUE;
			ptEntry.SetPhysicalAddress(physicalAddr);

			physicalAddr += PTEntryManageSize;
		}
		pageDirEntry.Present = TRUE;
		pageDirEntry.ReadWrite = TRUE;
		pageDirEntry.SetPageTableAddress(pageTable);
	}
}

void MappingKernelAddress(EFI_HANDLE ImageHandle, EFI_PHYSICAL_ADDRESS kernelMemBuffer,
	EFI_IMAGE_SECTION_HEADER* section, UINTN sectionCount, PDPTable& pdpTable)
{
	enum : uint64_t {
		KernelPML4EIndex = KernelImageBase / PML4EntryManageSize,
		KernelPML4ERest = KernelImageBase % PML4EntryManageSize,
		KernelPDPEIndex = KernelPML4ERest / PDPEntryManageSize,
		KernelPDPERest = KernelPML4ERest % PDPEntryManageSize,
		KernelPDEIndex = KernelPDPERest / PDEntryManageSize,
		KernelPDERest = KernelPDPERest % PDEntryManageSize,
		KernelPTEIndex = KernelPDERest / PTEntryManageSize,
		KernelPTERest = KernelPDERest % PTEntryManageSize
	};

	auto& kernelPageDir = *AllocatePageDirectory(ImageHandle);
	auto& kernelPageDirRef = pdpTable[KernelPDPEIndex];
	kernelPageDirRef.Present = TRUE;
	kernelPageDirRef.ReadWrite = TRUE;
	kernelPageDirRef.SetPageDirectoryAddress(kernelPageDir);

	uint8_t* physicalAddr = (uint8_t*)kernelMemBuffer;
	for (size_t i = 0; i < sectionCount; i++)
	{
		auto& curSection = section[i];
		if (curSection.SizeOfRawData)
		{
			auto dataSize = AlignSize(curSection.Misc.VirtualSize, EFI_PAGE_SIZE);

			// 起始 Page Table Index
			auto curPTIndex = curSection.VirtualAddress / PDEntryManageSize;
			auto restToMap = dataSize;
			uint8_t* startVirtualAddress = (uint8_t*)(KernelPDPEIndex * PDPEntryManageSize +
				curPTIndex * PDEntryManageSize);

			for (; restToMap; curPTIndex++)
			{
				auto& pageTableRef = kernelPageDir[curPTIndex];
				// 如果未分配则分配页表
				if (!pageTableRef.Present)
				{
					pageTableRef.SetPageTableAddress(*AllocatePageTable(ImageHandle));
					pageTableRef.Present = TRUE;
					pageTableRef.ReadWrite = TRUE;
				}
				PageTable& pageTable = pageTableRef.GetPageTableAddress();
				auto curPEIndex = (curSection.VirtualAddress % PDEntryManageSize)
					/ PTEntryManageSize;
				auto curVirtualAddress = startVirtualAddress + curPEIndex * PTEntryManageSize;


				MPrint(L"P: %lX, V: %lX\n", physicalAddr, curVirtualAddress);
				for (size_t j = curPEIndex; j < sizeof(pageTable) / sizeof(pageTable[0]); j++)
				{
					auto& ptEntry = pageTable[j];
					ptEntry.SetPhysicalAddress(physicalAddr);
					ptEntry.Present = TRUE;
					ptEntry.ReadWrite = TRUE;

					physicalAddr += EFI_PAGE_SIZE;
					curVirtualAddress += PTEntryManageSize;
					restToMap -= PTEntryManageSize;
					if (!restToMap)break;
				}
			}
		}
	}
}

void Bootloader::MappingFrameBuffer(PDPTable& pdpTable)
{
	auto frameBufferBase = gop->Mode->FrameBufferBase;
	auto PML4EIndex = frameBufferBase / PML4EntryManageSize;
	auto PML4ERest = frameBufferBase % PML4EntryManageSize;
	auto PDPEIndex = PML4ERest / PDPEntryManageSize;
	auto PDPERest = PML4ERest % PDPEntryManageSize;
	auto PDEIndex = PDPERest / PDEntryManageSize;
	auto PDERest = PDPERest % PDEntryManageSize;
	auto PTEIndex = PDERest / PTEntryManageSize;
	auto PTERest = PDERest % PTEntryManageSize;
	auto restToMap = AlignSize(PTERest + gop->Mode->FrameBufferSize, PTEntryManageSize);

	uint8_t* physicalAddr = (uint8_t*)frameBufferBase - PTERest;
	for (size_t i = PDPEIndex; restToMap; i++)
	{
		auto& pageDirRef = pdpTable[i];
		// 不存在页目录则分配
		if (!pageDirRef.Present)
		{
			pageDirRef.SetPageDirectoryAddress(*AllocatePageDirectory(imageHandle));
			pageDirRef.Present = TRUE;
			pageDirRef.ReadWrite = TRUE;
		}
		auto& pageDir = pageDirRef.GetPageDirectoryAddress();
		for (size_t j = PDEIndex; j < 512 && restToMap; j++)
		{
			auto& pageTableRef = pageDir[j];
			// 不存在页表则分配
			if (!pageTableRef.Present)
			{
				auto ptr = AllocatePageTable(imageHandle);
				pageTableRef.SetPageTableAddress(*ptr);
				pageTableRef.Present = TRUE;
				pageTableRef.ReadWrite = TRUE;
			}
			auto& pageTable = pageTableRef.GetPageTableAddress();
			for (size_t k = PTEIndex; k < 512 && restToMap; k++)
			{
				auto& ptEntry = pageTable[k];
				ptEntry.SetPhysicalAddress(physicalAddr);
				ptEntry.Present = TRUE;
				ptEntry.PCD = TRUE;
				ptEntry.ReadWrite = TRUE;

				physicalAddr += EFI_PAGE_SIZE;
				restToMap -= PTEntryManageSize;
			}
			PTEIndex = 0;
		}
		PDEIndex = 0;
	}
}

// 启用分页
void Bootloader::EnablePaging()
{
	EnableIA32ePaging(*pml4Table);
}

void Bootloader::PreparePaging()
{
	// 分配 PML4Table
	auto& pml4Table = *AllocatePML4Table(imageHandle);
	// 分配 PDPTable
	auto& pdpTable = *AllocatePDPTable(imageHandle);
	// 映射前 1 GB
	MappingLow1GB(imageHandle, pdpTable);
	// 映射视频缓冲
	MappingFrameBuffer(pdpTable);
	// 映射内核所在的 1 GB
	MappingKernelAddress(imageHandle, kernelMemBuffer, sectionStart, sectionCount, pdpTable);

	// 映射前 512 GB
	auto& pdpTableRef = pml4Table[0];
	pdpTableRef.SetPDPTableAddress(pdpTable);
	pdpTableRef.Present = TRUE;
	pdpTableRef.ReadWrite = TRUE;

	this->pml4Table = &pml4Table;
}