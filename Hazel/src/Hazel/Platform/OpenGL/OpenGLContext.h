#pragma once
#include "Hazel/Renderer/RenderContext.h"
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
