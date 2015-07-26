//
// Tomato OS
// [Internal] 字形提供程序
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#pragma once
#include "BootFont.h"

namespace Tomato {
	namespace OS {

		class GlyphProvider
		{
		public:
			GlyphProvider() = default;

			void Initialize(BitmapFont font);
			const unsigned char* GetGlyph(uint16_t chr) const noexcept;
		private:
			void GenerateIndexer();
		private:
			BitmapFont font;
			const unsigned char* indexes[UINT16_MAX];
		};
	}
}