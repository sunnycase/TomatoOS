//
// Tomato OS
// [Internal] 内部状态
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#pragma once
#include "../BootVideo/BootVideo.hpp"

namespace Tomato {
	namespace OS {

		extern BootVideo bootVideo;

		void KeFastFail(const wchar_t* message);
	}
}