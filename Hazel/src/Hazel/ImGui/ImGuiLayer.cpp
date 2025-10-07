#include "hzpch.h"
#include "ImGuiLayer.h"


#include "Hazel/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanImGuiLayer.h"

#include "Hazel/Renderer/RendererAPI.h"

#include <imgui.h>
namespace Hazel {

	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return new VulkanImGuiLayer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
	}

	void ImGuiLayer::SetDarkThemeV2Colors()
	{
	}

	void ImGuiLayer::AllowInputEvents(bool allowEvents)
	{
		
	}

}
