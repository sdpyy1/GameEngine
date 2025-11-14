#pragma once
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Core/RenderThread.h"
#include "Hazel/ImGui/ImGuiRendererManager.h"
namespace Hazel
{
	class RendererManager
	{
	public:
        RendererManager();
		void ExecutePreFrame();
		void Init();
		void tick(float timestep, bool m_Minimized);
		void RenderImGui();
		LayerStack& GetLayerStatck() {return m_LayerStack;};

	private:
		RenderThread m_RenderThread;
		std::shared_ptr<ImGuiRendererManager> m_ImGuiLayer;
		LayerStack m_LayerStack;

	};
}

