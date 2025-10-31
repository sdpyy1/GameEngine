#include "hzpch.h"
#include "Hazel/Renderer/RenderContext.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Core/Application.h>
#include <Platform/OpenGL/OpenGLContext.h>

namespace Hazel {
	Ref<RenderContext> RenderContext::Create(void* window)
	{
		switch (Renderer::Current())
		{
		case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanContext>::Create(window);
		case RendererAPI::Type::OpenGL:  return Ref<OpenGLContext>::Create(window);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
