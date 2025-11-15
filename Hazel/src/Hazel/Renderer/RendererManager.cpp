#include "hzpch.h"
#include "RendererManager.h"
#include "Renderer.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/SceneRender.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Core/Application.h"
namespace Hazel {
	RendererManager::RendererManager() :m_RenderThread(ThreadingPolicy::SingleThreaded)
	{
		m_RenderThread.Run();

		Renderer::Init();
		m_RenderThread.Pump();


		m_ImGuiRendererManager = ImGuiRendererManager::Create();
		m_ImGuiRendererManager->pre();
		m_SceneRender = std::make_shared<SceneRender>();
		m_RenderThread.Pump();
	}


	void RendererManager::tick(float timestep)
	{
		// LOG_TRACE("RenderManager::tick {}",Renderer::GetCurrentFrameIndex());
		Application::GetSceneManager()->GetActiveScene()->CollectRenderableEntities(m_SceneRender);

		Renderer::BeginFrame();

		m_SceneRender->PreRender(Application::GetSceneManager()->GetActiveScene()->GetSceneInfo());
		m_SceneRender->EndRender();

		RenderImGui();

		Renderer::EndFrame();
		// LOG_TRACE("RenderManager::tick Done!");
	}


	void RendererManager::RenderImGui()
	{
		RENDER_SUBMIT([this]()
			{
				m_ImGuiRendererManager->Tick(0,nullptr);
			});
	}

	void RendererManager::ExecutePreFrame()
	{
		m_RenderThread.BlockUntilRenderComplete();
		// �˴���Ⱦ�̺߳����߳�ͬ����
		// xxxx
		m_RenderThread.NextFrame();
		m_RenderThread.Kick(); // ��ʼ��Ⱦ��һ֡
	}

	bool RendererManager::OnEvent(Event& e)
	{
		bool isHandle = false;
		isHandle = m_ImGuiRendererManager->OnEvent(e);
		return isHandle;
	}

	Ref<Hazel::Image2D> RendererManager::GetTextureWhichNeedDebug() const
	{
		return m_SceneRender->GetTextureWhichNeedDebug();
	}

	Ref<Hazel::Image2D> RendererManager::GetFinalImage() const
	{
		return m_SceneRender->GetFinalImage();
	}

}
