#include "hzpch.h"
#include "Hazel/Renderer/GraphicsContext.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Core/Application.h>

namespace Hazel {

	Scope<RenderContext> RenderContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::RenderAPI::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::RenderAPI::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
			case RendererAPI::RenderAPI::Vulkan:  return CreateScope<VulkanContext>(static_cast<GLFWwindow*>(window));
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
