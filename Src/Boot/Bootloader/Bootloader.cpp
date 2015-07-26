//
// Tomato OS
// Bootloader
// (c) 2015 SunnyCase
// 创建日期：2015-4-18
#include "stdafx.h"
#include "bootloader.h"

static const wchar_t KernelFilePath[] = LR"(Tomato\System\OSKernel.exe)";

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	Bootloader loader(ImageHandle);
	loader.Boot();

	return EFI_SUCCESS;
}

KernelFile::KernelFile(KernelFile && file)
	:imageHandle(file.imageHandle), kernelFileBuffer(file.kernelFileBuffer)
{
	file.kernelFileBuffer = nullptr;
}

KernelFile::~KernelFile()
{
	if (kernelFileBuffer)
		FreePool(kernelFileBuffer);
}

void KernelFile::LoadKernelFile()
{
	EFI_LOADED_IMAGE* loadedImage;
	EFI_FILE_IO_INTERFACE* volume;
	// 获取 Loader 所在的卷
	gBS->HandleProtocol(imageHandle, &gEfiLoadedImageProtocolGuid, (void**)&loadedImage);
	gBS->HandleProtocol(loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&volume);

	EFI_FILE_HANDLE rootFS, fileHandle;
	volume->OpenVolume(volume, &rootFS);
	// 读取文件
	EXIT_IF_NOT_SUCCESS(rootFS->Open(rootFS, &fileHandle, (CHAR16*)KernelFilePath, EFI_FILE_MODE_READ, 0),
		imageHandle, L"Cannot Open Tomato Kernel File.\r\n");

	UINT8* kernelBuffer = (UINT8*)AllocatePool(KernelPoolSize);
	EXIT_IF_NOT_SUCCESS(fileHandle->Read(fileHandle, &KernelPoolSize, kernelBuffer),
		imageHandle, L"Cannot Read Tomato Kernel File.\r\n");
	fileHandle->Close(fileHandle);

	kernelFileBuffer = kernelBuffer;
}

KernelEntryPoint KernelFile::GetEntryPoint()
{
	auto dosHeader = (EFI_IMAGE_DOS_HEADER*)kernelFileBuffer;
	auto* ntHeaders = (EFI_IMAGE_NT_HEADERS64*)(kernelFileBuffer + dosHeader->e_lfanew);
	return (KernelEntryPoint)(ntHeaders->OptionalHeader.ImageBase +
		ntHeaders->OptionalHeader.AddressOfEntryPoint);
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

void Bootloader::Boot()
{
	MPrint(L"Loading Tomato OS...\r\n");

	SetGraphicsMode();
	KernelEntryPoint entryPoint;
	{
		auto file = LoadKernel();
		entryPoint = file.GetEntryPoint();
		PrepareKernel(file);
	}
	PreparePaging();
	InitializeKernelEntryParams();
	PrepareVirtualMemoryMapping();
	EnablePaging();

	RunKernel(entryPoint);
}

EFI_MEMORY_DESCRIPTOR* GetMemoryMap(UINTN& entries, UINTN& mapKey, UINTN& descriptorSize, UINT32& descriptorVersion)
{
	UINTN totalSize = 0;
	EFI_MEMORY_DESCRIPTOR* descriptor;
	gBS->GetMemoryMap(&totalSize, nullptr, &mapKey, &descriptorSize, &descriptorVersion);
	descriptor = (EFI_MEMORY_DESCRIPTOR*)AllocatePool(totalSize);
	gBS->GetMemoryMap(&totalSize, descriptor, &mapKey, &descriptorSize, &descriptorVersion);
	entries = totalSize / descriptorSize;
	return descriptor;
}

void Bootloader::PrintMemoryMap()
{
	UINTN entries, mapKey, descriptorSize;
	UINT32 descriptorVersion;

	EFI_MEMORY_DESCRIPTOR* descriptor = GetMemoryMap(entries, mapKey, descriptorSize, descriptorVersion);

	MPrint(L"  Type        Start Address     End Address       Virtual Address      \n");
	MPrint(L"  ==========  ================  ================  ================\n");
	EFI_MEMORY_DESCRIPTOR* MemoryMapEntry = descriptor;
	for (UINTN i = 0; i < entries; i++) {
		MPrint(L"  %s  %lX  %lX  %lX\n",
			MemoryMapEntry->Type == KernelPoolType ? L"KernelPool" : MemoryMapEntry->Type == KernelPagingDataType ? L"KernelPage" : OsLoaderMemoryTypeDesc[MemoryMapEntry->Type],
			MemoryMapEntry->PhysicalStart,
			MemoryMapEntry->PhysicalStart + LShiftU64(MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT) - 1,
			MemoryMapEntry->VirtualStart);
		MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR(MemoryMapEntry, descriptorSize);
	}
	FreePool(descriptor);
}

bool KernelFile::ValidateKernel()
{
	auto* dosHeader = (EFI_IMAGE_DOS_HEADER*)kernelFileBuffer;
	if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		return FALSE;
	auto* ntHeaders = (EFI_IMAGE_NT_HEADERS64*)(kernelFileBuffer + dosHeader->e_lfanew);
	if (ntHeaders->Signature != EFI_IMAGE_NT_SIGNATURE)
		return FALSE;
	return TRUE;
}

KernelFile Bootloader::LoadKernel()
{
	KernelFile file(imageHandle);

	file.LoadKernelFile();
	return file;
}

void Bootloader::RunKernel(KernelEntryPoint entryPoint)
{
	entryPoint(params);
}

enum
{
	DESIRED_HREZ = 800,
	DESIRED_VREZ = 600,
	DESIRED_PIXEL_FORMAT = EFI_GRAPHICS_PIXEL_FORMAT::PixelBlueGreenRedReserved8BitPerColor
};

void Bootloader::InitializeKernelEntryParams()
{
	/*EfiGetSystemConfigurationTable(&, (void**)&params.AcpiTable);
	EfiGetSystemConfigurationTable(&SMBIOSTableGuid, (void**)&params.SmbiosTable);*/
	params.RT = gRT;
	params.FrameBufferBase = gop->Mode->FrameBufferBase;
	params.FrameWidth = DESIRED_HREZ;
	params.FrameHeight = DESIRED_VREZ;
	params.FrameBufferSize = gop->Mode->FrameBufferSize;
}

UINTN GetAllSectionsMemoryPages(EFI_IMAGE_SECTION_HEADER* first, UINTN count)
{
	UINTN size = 0;
	for (UINTN i = 0; i < count; i++, first++)
		size += AlignSize(first->Misc.VirtualSize, EFI_PAGE_SIZE);
	return size / EFI_PAGE_SIZE;
}

void Bootloader::PrepareVirtualMemoryMapping()
{
	UINTN entries, mapKey, descriptorSize;
	UINT32 descriptorVersion;

	EFI_MEMORY_DESCRIPTOR* descriptor = GetMemoryMap(entries, mapKey, descriptorSize, descriptorVersion);
	gBS->ExitBootServices(imageHandle, mapKey);

	EFI_MEMORY_DESCRIPTOR* memoryMapEntry = descriptor;
	for (UINTN i = 0; i < entries; i++)
	{
		if (memoryMapEntry->Attribute & EFI_MEMORY_RUNTIME)
		{
			memoryMapEntry->VirtualStart = memoryMapEntry->PhysicalStart;
		}
		memoryMapEntry = NEXT_MEMORY_DESCRIPTOR(memoryMapEntry, descriptorSize);
	}

	EFI_STATUS status = gRT->SetVirtualAddressMap(entries * descriptorSize, descriptorSize,
		EFI_MEMORY_DESCRIPTOR_VERSION, descriptor);
	if (EFI_ERROR(status))
		gRT->ResetSystem(EfiResetWarm, EFI_LOAD_ERROR, 62, (CHAR16*)L"Setting Memory mapping failed.");

	params.MemoryDescriptor = descriptor;
	params.MemoryDescriptorSize = descriptorSize;
	params.MemoryDescriptorEntryCount = entries;
}

void Bootloader::SetGraphicsMode()
{
	EFI_HANDLE* handleBuffer;
	UINTN handleCount = 0;
	UINTN sizeOfInfo;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gopModeInfo;
	gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, nullptr, &handleCount, &handleBuffer);
	gBS->HandleProtocol(handleBuffer[0], &gEfiGraphicsOutputProtocolGuid, (void**)&gop);

	size_t modeIndex = 0;
	for (;; modeIndex++) {
		gop->QueryMode(gop, modeIndex, &sizeOfInfo, &gopModeInfo);
		if (gopModeInfo->HorizontalResolution == DESIRED_HREZ &&
			gopModeInfo->VerticalResolution == DESIRED_VREZ &&
			gopModeInfo->PixelFormat == DESIRED_PIXEL_FORMAT)
			break;
	}
	gop->SetMode(gop, modeIndex);

	MPrint(L"Change Graphics Mode: %dx%d\n", DESIRED_HREZ, DESIRED_VREZ);
}

void Bootloader::PrepareKernel(KernelFile& file)
{
	if (file.ValidateKernel())
	{
		auto kernelImageBase = file.GetKernelFileBuffer();
		auto* dosHeader = (EFI_IMAGE_DOS_HEADER*)kernelImageBase;
		auto ntHeaders = (EFI_IMAGE_NT_HEADERS64*)(kernelImageBase + dosHeader->e_lfanew);

		sectionStart = (EFI_IMAGE_SECTION_HEADER*)(((UINT8*)ntHeaders) + sizeof(EFI_IMAGE_NT_HEADERS64)
			- sizeof(EFI_IMAGE_OPTIONAL_HEADER64) + ntHeaders->FileHeader.SizeOfOptionalHeader);
		sectionCount = ntHeaders->FileHeader.NumberOfSections;
		UINTN sectionAlign = ntHeaders->OptionalHeader.SectionAlignment;
		UINTN fileAlign = ntHeaders->OptionalHeader.FileAlignment;

		UINTN memAllocPages = GetAllSectionsMemoryPages(sectionStart, sectionCount);
		EXIT_IF_NOT_SUCCESS(gBS->AllocatePages(AllocateAnyPages, KernelPoolType, memAllocPages,
			&kernelMemBuffer), imageHandle, L"Cannot allocate Kernel Memory.\r\n");

		EFI_IMAGE_SECTION_HEADER* section = sectionStart;
		UINT8* memBuffer = (UINT8*)kernelMemBuffer;
		MPrint(L"  Name        Virtual Address     Raw Address       Raw Size      Mem Size\n");
		for (UINTN i = 0; i < sectionCount; i++, section++)
		{
			BOOLEAN dataInFile = section->PointerToRawData != 0;
			UINT8* sectionData = kernelImageBase + section->PointerToRawData;
			UINTN memAllocSize = AlignSize(section->Misc.VirtualSize, EFI_PAGE_SIZE);

			if (memAllocSize)
			{
				if (dataInFile)
					CopyMem(memBuffer, sectionData, section->SizeOfRawData);
				memBuffer += memAllocSize;
			}
			MPrint(L"  ");
			AsciiPrint((CHAR8*)section->Name);
			MPrint(L"     %lX      %lX    %lX    %lX\r\n", section->VirtualAddress, section->PointerToRawData,
				section->SizeOfRawData, memAllocSize);
		}
	}
	else
		EXIT_IF_NOT_SUCCESS(EFI_LOAD_ERROR, imageHandle, L"Invalid Tomato Kernel Image.\r\n");
}