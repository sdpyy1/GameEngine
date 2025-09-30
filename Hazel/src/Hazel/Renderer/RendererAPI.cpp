#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Hazel {

	RendererAPI::RenderAPI RendererAPI::s_API = RendererAPI::RenderAPI::Vulkan;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
			case RendererAPI::RenderAPI::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::RenderAPI::OpenGL:  return CreateScope<OpenGLRendererAPI>();
			case RendererAPI::RenderAPI::Vulkan:  return CreateScope<OpenGLRendererAPI>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
