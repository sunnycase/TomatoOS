//
// Tomato OS
// 进程管理器
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#include "stdafx.h"
#include "ProcessManager.h"

using namespace Tomato::OS;

ProcessManager::ProcessManager()
{
	auto c = malloc(1);
	free(c);
}

void ProcessManager::AddProcess(Process && process)
{
}

Process::Process()
{
}
