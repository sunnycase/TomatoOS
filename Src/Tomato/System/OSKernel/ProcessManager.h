//
// Tomato OS
// [Internal] 进程管理器
// (c) 2015 SunnyCase
// 创建日期：2015-4-22
#pragma once

namespace Tomato {
	namespace OS {

		class Process
		{
		public:
			Process();
		};

		class ProcessManager
		{
		public:
			ProcessManager();

			void AddProcess(Process&& process);
		};
	}
}