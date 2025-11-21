#pragma once
#include "Hazel/Renderer/old/RenderContext.h"
struct GLFWwindow;
namespace GameEngine {
	class OpenGLContext : public RenderContext
	{
	public:
		OpenGLContext(void* windowHandle);
		virtual void Init() override;
	private:
		void* m_WindowHandle;
	};

}
