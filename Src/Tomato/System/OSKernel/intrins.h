//
// Tomato OS
// [Internal] 指令
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#pragma once

extern "C" {
	// 设置 RSP 并跳转
	extern __declspec(noreturn) void setrspjmp(void* ptr, void(*func)());
}