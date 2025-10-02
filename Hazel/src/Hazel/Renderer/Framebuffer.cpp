#include "hzpch.h"
#include "Framebuffer.h"

#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanFramebuffer.h>

namespace Hazel {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Ref<Framebuffer> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:		return nullptr;
		case RendererAPI::Type::Vulkan:	result = Ref<VulkanFramebuffer>::Create(spec); break;
		}
		return result;
	}

}
