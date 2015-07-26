//
// Tomato OS
// [Internal] 内核服务
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#include "stdafx.h"
#include "KernelService.h"
#include "InternelStates.h"

using namespace Tomato::OS;

KernelService::KernelService()
{
}

void KernelService::Run()
{
	bootVideo.ClearScreen();
	bootVideo.MovePositionTo(20, 20);
	bootVideo.PutString(L"Entering Kernel...\r\n");

	auto ptr = malloc(100);
	bootVideo.PutFormat(L"Allocate 100B at %x\r\n", ptr); 
	ptr = malloc(100);
	bootVideo.PutFormat(L"Allocate 100B at %x\r\n", ptr);
	
	__halt();
}
