#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Hazel {

	RendererAPI::APIType RendererAPI::s_API = RendererAPI::APIType::Vulkan;

	Scope<RendererAPI> RendererAPI::Create_old()
	{
		switch (s_API)
		{
			case RendererAPI::APIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::APIType::OpenGL:  return CreateScope<OpenGLRendererAPI>();
			case RendererAPI::APIType::Vulkan:  return CreateScope<OpenGLRendererAPI>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
