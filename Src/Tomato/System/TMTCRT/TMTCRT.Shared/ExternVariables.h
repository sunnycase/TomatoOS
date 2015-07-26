//
// Tomato OS - C 运行时库
// 外部变量
// (c) 2015 SunnyCase
// 创建日期：2015-4-23
#pragma once

#ifdef CRT_KERNEL
#include "../../../System/OSKernel/MemoryManager.h"
#include "../../BootVideo/BootVideo.hpp"
#endif

namespace Tomato {
	namespace CRT {
#ifdef CRT_KERNEL
		extern Tomato::OS::BootVideo* bootVideo;
		extern Tomato::OS::MemoryManager* memoryManager;
#endif
	}
}