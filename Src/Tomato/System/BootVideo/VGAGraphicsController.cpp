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
		AddressRegister = 0x3CE,
		DataRegister = 0x3CF
	};

	void delay()
	{
		for (size_t i = 0; i < 10000; i++);
	}

#pragma pack(push, 1)

	template<uint8_t Index>
	struct RegAccessor
	{
	protected:
		static uint8_t ReadValue()
		{
			__outbyte(AddressRegister, Index);
			delay();
			return __inbyte(DataRegister);
		}

		static void WriteValue(uint8_t value)
		{
			__outbyte(AddressRegister, Index);
			delay();
			__outbyte(DataRegister, value);
			delay();
		}
	};

#define DEFINE_GRAPHICSREG_FUNCS(name) \
	name(uint8_t value)				   \
		:value(value) {}			   \
									   \
	static name Read()				   \
	{								   \
		return ReadValue();			   \
	}								   \
									   \
	void Write()					   \
	{								   \
		WriteValue(value);			   \
	}

	// 显示模式寄存器
	struct tagGraphicsModeReg : RegAccessor<0x5>
	{
		union
		{
			struct
			{
				uint8_t WriteMode : 2;
				uint8_t : 1;
				uint8_t ReadMode : 1;
				uint8_t HostOE : 1;
				uint8_t ShiftReg : 1;
				uint8_t Shift256 : 1;
			};
			uint8_t value;
		};
		DEFINE_GRAPHICSREG_FUNCS(tagGraphicsModeReg);
	};
	static_assert(sizeof(tagGraphicsModeReg) == 1, "size of tagGraphicsModeReg must be 1.");

	// 内存映射类别
	enum MemoryMapKind : uint8_t
	{
		A0000h_BFFFFh = 0,	// 128K
		A0000h_AFFFFh = 1,	// 64K
		B0000h_B7FFFh = 2,	// 32K
		B8000h_BFFFFh = 3	// 32K
	};

	// Graphics Controller 杂项寄存器
	struct tagMiscellaneousGraphicsReg : RegAccessor<0x6>
	{
		union
		{
			struct
			{
				uint8_t AlphaDisable : 1;		// 关闭字符模式，否则图形模式
				uint8_t ChainOE: 1;
				MemoryMapKind MemoryMap : 2;
				uint8_t : 4;
			};
			uint8_t value;
		};
		DEFINE_GRAPHICSREG_FUNCS(tagMiscellaneousGraphicsReg);
	};
	static_assert(sizeof(tagMiscellaneousGraphicsReg) == 1, "size of tagMiscellaneousGraphicsReg must be 1.");

#pragma pack(pop)
}

uint8_t VGAGraphicsController::ReadMode()
{
	auto value = tagMiscellaneousGraphicsReg::Read();
	value.AlphaDisable = 1;
	value.MemoryMap = MemoryMapKind::B8000h_BFFFFh;
	value.Write();

	return tagMiscellaneousGraphicsReg::Read().value;
}