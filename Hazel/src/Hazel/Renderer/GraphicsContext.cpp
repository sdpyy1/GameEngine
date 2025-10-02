#include "hzpch.h"
#include "Hazel/Renderer/GraphicsContext.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Core/Application.h>

namespace Hazel {

	Scope<RenderContext> RenderContext::Create_old(void* window)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::Type::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
			case RendererAPI::Type::Vulkan:  return CreateScope<VulkanContext>(static_cast<GLFWwindow*>(window));
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
