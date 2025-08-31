#pragma once
namespace Engine {
	class GraphicsContext
	{
	public:
		// 定义各种API如何初始化上下文和如何交换缓冲区
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};
};