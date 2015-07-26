//
// Tomato OS
// [Internal] 数学辅助
// (c) 2015 SunnyCase
// 创建日期：2015-4-32
#pragma once
#include <cstdint>

// 向上取整除法
inline size_t CeilDiv(size_t numerator, size_t denominator)
{
	auto factor = numerator / denominator;
	if (numerator % denominator)
		factor++;
	return factor;
}

inline bool TestBit(uint64_t value, int index)
{
	return (value & (1ui64 << index)) != 0;
}