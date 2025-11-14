#include "hzpch.h"
#include "RendererManager.h"
#include "Renderer.h"
#include <Hazel/Editor/EditorLayer.h>
namespace Hazel {
	void RendererManager::Init()
	{
		m_RenderThread.Run();
		Renderer::Init();
		m_RenderThread.Pump();
		auto a = new EditorLayer();

		m_ImGuiLayer = ImGuiRendererManager::Create();

		m_LayerStack.PushLayer(a);
		a->OnAttach();
	}

	void RendererManager::tick(float timestep,bool m_Minimized)
	{

		// 这里就做一件事：交换Submit用的命令缓冲区的Index，用下一个命令记录新命令。 这个切换只是用于多线程渲染的缓冲区，和渲染内部的Index无关
		m_RenderThread.NextFrame();

		// -----------------同步点结束------------------------------------------------

		// 提醒渲染线程工作，渲染上一帧信息
		m_RenderThread.Kick();

		// 上一行和下一行表示GPU和渲染线程都开始工作了，在渲染线程渲染上一帧的时候,CPU开始收集下一帧渲染命令
		if (!m_Minimized)
		{
			// 重置DrawCall=0，（交换链的Begin也写在这里了：获取下一帧图片索引、更新交换链FrameIndex、清空交换链的命令缓冲区)
			Renderer::BeginFrame();

			// 更新各层
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);
			}

			RenderImGui();

			// 提交命令缓冲区、呈现图片
			Renderer::EndFrame();
		}
	}

	RendererManager::RendererManager():m_RenderThread(ThreadingPolicy::SingleThreaded)
	{
		Init();
	}

	void RendererManager::RenderImGui()
	{
		Renderer::Submit([this]()
			{
				m_ImGuiLayer->Begin();

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender(); {
				}

				m_ImGuiLayer->End();
			});
	}

	void RendererManager::ExecutePreFrame()
	{
		m_RenderThread.BlockUntilRenderComplete();
	}

}
