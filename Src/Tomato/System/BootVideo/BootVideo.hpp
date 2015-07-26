//
// Tomato OS
// [Internal] 引导阶段视频控制
// (c) 2015 SunnyCase
// 创建日期：2015-4-20
#pragma once
#include "GlyphProvider.h"
#include <stdint.h>

namespace Tomato {
	namespace OS
	{
		// VGA 视频客户端
		class VGAVideoClient
		{
		public:
			int ConsoleWrite(unsigned char ch);
			void ConsoleClear(const unsigned short c);
			unsigned ConsoleSetColor(const unsigned c);
		};

		class VGAGraphicsController
		{
		public:
			uint8_t ReadMode();
		};

		class VGAExternalController
		{
		public:
			uint8_t ReadMode();
		};

		class BootVideo
		{
		public:
			BootVideo() = default;

			void Initialize(uint32_t* frameBuffer, size_t bufferSize, size_t frameWidth, size_t frameHeight);
			void PutChar(wchar_t chr);
			void PutString(const wchar_t* string);
			void PutString(const wchar_t* string, size_t count);
			void PutFormat(const wchar_t* format, ...);
			void MovePositionTo(size_t x, size_t y);
			void ClearScreen();
			void SetBackground(uint32_t color);
		private:
			void FixCurrentFramePointer();
		private:
			GlyphProvider glyphProvider;
			uint32_t* frameBuffer, *currentFramePointer;
			size_t bufferSize;
			size_t frameWidth, frameHeight;
			size_t currentX, currentY;
			uint32_t foreground, background;
		};
	}
}