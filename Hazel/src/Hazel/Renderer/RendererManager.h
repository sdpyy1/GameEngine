#pragma once
#include "Hazel/Core/RenderThread.h"
#include "Hazel/Editor/ImGuiRendererManager.h"
namespace Hazel
{
	class Scene;
	class SceneRender;
	class RendererManager
	{
	public:
        RendererManager();
		~RendererManager() = default;
		void Tick(float timestep);
		bool OnEvent(Event& e);
		void ExecutePreFrame();
		void RenderImGui(float timestep);
		Ref<Hazel::Image2D>  GetFinalImage() const;
		Ref<Image2D> GetTextureWhichNeedDebug() const;
	private:
		RenderThread m_RenderThread;
		std::shared_ptr<ImGuiRendererManager> m_ImGuiRendererManager;
		std::shared_ptr<SceneRender> m_SceneRender;
	};
}

