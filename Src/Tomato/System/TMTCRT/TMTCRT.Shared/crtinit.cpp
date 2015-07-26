//
// Tomato OS - C 运行时库
// 初始化
// (c) 2015 SunnyCase
// 创建日期：2015-4-23
#include "stdafx.h"
#include "crtinit.h"

void Tomato::CRT::_crt_init()
{
	_crt_init_heap();
}