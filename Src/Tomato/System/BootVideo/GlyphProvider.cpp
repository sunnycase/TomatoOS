//
// Tomato OS
// 字形提供程序
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#include "stdafx.h"
#include "GlyphProvider.h"

using namespace Tomato::OS;

void GlyphProvider::Initialize(BitmapFont font)
{
	this->font = font;
	GenerateIndexer();
}

const unsigned char * GlyphProvider::GetGlyph(uint16_t chr) const noexcept
{
	if (chr > font.Chars)
		return font.Bitmap;
	return indexes[chr];
}

void GlyphProvider::GenerateIndexer()
{
	auto index = font.Index;
	auto bitmap = font.Bitmap;
	auto height = font.Height;
	for (size_t i = 0; i < UINT16_MAX && i < font.Chars; i++)
	{
		if (i == *index)
		{
			indexes[i] = bitmap;
			index++;
			bitmap += height;
		}
		else
			indexes[i] = font.Bitmap;
	}
}
