#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include <Platform/Vulkan/VulkanContext.h>

#include "Hazel/Core/Input.h"
#include "Hazel/Utils/PlatformUtils.h"
#include <Platform/Windows/WindowsWindow.h>
#include "Hazel/Asset/AssetImporter.h"
#include <nfd.hpp>

namespace Hazel {

	Application* Application::s_Instance = nullptr;
	static std::thread::id s_MainThreadID;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification), m_RenderThread(ThreadingPolicy::SingleThreaded)
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		s_MainThreadID = std::this_thread::get_id();
		m_RenderThread.Run();

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(WindowProps(m_Specification.Name,1600,900));  // 这里创建了GLFW窗口、也初始化了RenderContext和SwapChain
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));
		HZ_CORE_ASSERT(NFD::Init() == NFD_OKAY);

		Renderer::Init();
		// 执行当前的Renderer命令
		m_RenderThread.Pump();
		AssetImporter::Init();
		//ImGui初始化
		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}
	}


	void Application::Run()
	{
		HZ_PROFILE_FUNCTION();

		while (m_Running)
		{
			// 阻塞等待渲染线程
			m_RenderThread.BlockUntilRenderComplete();  

			// -----------------同步点：到这里渲染线程和主线程同步----------------------
			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			// 这里就做一件事：交换Submit用的命令缓冲区的Index，用下一个命令记录新命令
			m_RenderThread.NextFrame();
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

				// GUI渲染
				Application* app = this;
				if (m_Specification.EnableImGui)
				{
					Renderer::Submit([app]() { app->RenderImGui(); });
				}

				// 提交命令缓冲区、呈现图片
				Renderer::EndFrame();  
			}
			// 主线程 FrameIndex 更新
			m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;

			m_Window->OnUpdate();
			ExecuteMainThreadQueue();

		}
	}

	Application::~Application()
	{
		HZ_PROFILE_FUNCTION();
		m_RenderThread.Terminate();

		//ScriptEngine::Shutdown();
		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::RenderImGui()
	{
		// ImGUI渲染前需要的准备函数
		m_ImGuiLayer->Begin();
		// 渲染各层的ImGUI窗口
		for (Layer* layer : m_LayerStack)
			layer->OnImGuiRender();
		m_ImGuiLayer->End();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::SubmitToMainThread(const std::function<void()>& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(function);
	}


	void Application::OnEvent(Event& e)
	{
		HZ_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<WindowMinimizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowMinimize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}
	bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		m_Minimized = e.IsMinimized();

		return false;

	}
	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			//m_Minimized = true;
			return false;
		}
		//m_Minimized = false;

		auto& window = m_Window;
		Renderer::Submit([&window, width, height]() mutable
			{
				window->GetSwapChain().OnResize(width, height);
			});

		return false;
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

}
