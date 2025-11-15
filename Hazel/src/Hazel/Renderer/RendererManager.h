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
		void tick(float timestep);
		bool OnEvent(Event& e);
		void ExecutePreFrame();
		void RenderImGui();
		Ref<Hazel::Image2D>  GetFinalImage() const;
		Ref<Image2D> GetTextureWhichNeedDebug() const;
	private:
		RenderThread m_RenderThread;
		std::shared_ptr<ImGuiRendererManager> m_ImGuiRendererManager;
		std::shared_ptr<SceneRender> m_SceneRender;
	};
}

