#pragma once
#include "Hazel/Editor/ImGuiRendererManager.h"
#include "Hazel/Renderer/RenderCommandBuffer.h"


namespace Hazel {

	class VulkanImGuiLayer : public ImGuiRendererManager
	{
	public:
		VulkanImGuiLayer();
		VulkanImGuiLayer(const std::string& name);
		virtual ~VulkanImGuiLayer();

		virtual void Begin() override;
		virtual void End() override;

	private:
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		float m_Time = 0.0f;
	};

}
