//
// Tomato OS
// [Internal] 内核入口
// (c) 2015 SunnyCase
// 创建日期：2015-4-18
#include "stdafx.h"
#include "../BootVideo/BootVideo.hpp"
#include "InternelStates.h"
#include "MemoryManager.h"
#include "../../../include/kernel/mmu.hpp"
#include "../../../include/kernel/kernel.h"
#include "intrins.h"
#include "KernelService.h"
#include "../TMTCRT/TMTCRT.Shared/ExternVariables.h"
#include "../TMTCRT/TMTCRT.Shared/crtinit.h"

Tomato::OS::MemoryManager* Tomato::CRT::memoryManager = nullptr;
Tomato::OS::BootVideo* Tomato::CRT::bootVideo = nullptr;

using namespace Tomato::OS;

extern "C" {
	void __cdecl __chkstk()
	{

	}
}

BootVideo Tomato::OS::bootVideo;
MemoryManager memoryManager;

__declspec(noreturn) void Main();

void __cdecl KernelMain(KernelEntryParams params)
{
	bootVideo.Initialize((uint32_t*)params.FrameBufferBase, params.FrameBufferSize,
		params.FrameWidth, params.FrameHeight);
	Tomato::CRT::bootVideo = &bootVideo;
	bootVideo.SetBackground(0x00222222);
	bootVideo.ClearScreen();
	bootVideo.MovePositionTo(20, 20);
	bootVideo.PutString(L"Loading Tomato OS...\r\n");

	memoryManager.Initialize(params.MemoryDescriptor, params.MemoryDescriptorSize,
		params.MemoryDescriptorEntryCount);
	Tomato::CRT::memoryManager = &memoryManager;

	// 分配栈空间
	enum : size_t
	{
		stackSize = 64 * 1024, // 64 KB
		stackPages = stackSize / PAGE_SIZE
	};
	auto stackPtr = (uint8_t*)memoryManager.AllocatePages(stackPages) + stackSize;

	bootVideo.PutFormat(L"Allocate %d Pages Stack, At %x\r\n", stackPages, stackPtr);
	// 设置栈并跳转
	setrspjmp(stackPtr, Main);
}

void Main()
{
	Tomato::CRT::_crt_init();
	KernelService kernel;
	kernel.Run();
}

static_assert(std::is_same<KernelEntryPoint, decltype(&KernelMain)>::value, "KernelMain must be a KernelEntryPoint.");

void Tomato::OS::KeFastFail(const wchar_t* message)
{
	bootVideo.ClearScreen();
	bootVideo.MovePositionTo(20, 20);
	bootVideo.PutString(message);
	__halt();
}