#include "hzpch.h"
#include "Hazel/Renderer/Framebuffer_old.h"

#include "Hazel/Renderer/Renderer.h"


namespace Hazel {
	
	Ref_old<Framebuffer_old> Framebuffer_old::Create_old(const FramebufferSpecification_old& spec)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}

