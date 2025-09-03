#include "pch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Engine {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		ENGINE_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		ENGINE_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ENGINE_CORE_ASSERT(status, "Failed to initialize Glad!");

		ENGINE_CORE_INFO("OpenGL Info:");
		ENGINE_CORE_INFO("  Vendor: {0}", static_cast<const void*> (glGetString(GL_VENDOR)));
		ENGINE_CORE_INFO("OpenGL Version: {}", static_cast<const void*>(glGetString(GL_VERSION)));
		ENGINE_CORE_INFO("  Version: {0}", static_cast<const void*>(glGetString(GL_VERSION)));

#ifdef ENGINE_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		ENGINE_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Hazel requires at least OpenGL version 4.5!");
#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		ENGINE_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}