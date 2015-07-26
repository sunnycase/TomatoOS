//
// Tomato OS
// [Internal] 引导阶段视频控制
// (c) 2015 SunnyCase
// 创建日期：2015-4-21
#include "stdafx.h"
#include "BootVideo.hpp"

using namespace Tomato::OS;

namespace
{
	enum : uint16_t
	{
		MiscellaneousOutputReadRegister = 0x3CC,
		DataRegister = 0x3CF
	};

	void delay()
	{
		for (size_t i = 0; i < 10000; i++);
	}

#pragma pack(push, 1)

	// 输出杂项寄存器
	struct tagMiscellaneousOutputReg
	{
		union
		{
			struct
			{
				uint8_t IOAS : 1;
				uint8_t RAMEnable: 1;
				uint8_t ColorSelect : 2;
				uint8_t : 1;
				uint8_t OEPage : 1;
				uint8_t HSYNCP : 1;
				uint8_t VSYNCP : 1;
			};
			uint8_t value;
		};

		tagMiscellaneousOutputReg(uint8_t value)
			:value(value) {}

		static tagMiscellaneousOutputReg Read()
		{
			return __inbyte(MiscellaneousOutputReadRegister);
		}
	};
	static_assert(sizeof(tagMiscellaneousOutputReg) == 1, "size of tagGraphicsModeReg must be 1.");

#pragma pack(pop)
}

uint8_t VGAExternalController::ReadMode()
{
	return tagMiscellaneousOutputReg::Read().value;
}