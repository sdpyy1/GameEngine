#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include <Platform/Vulkan/VulkanContext.h>

#include "Hazel/Core/Input.h"
#include "Hazel/Utils/PlatformUtils.h"
#include <Platform/Windows/WindowsWindow.h>
#include <nfd.hpp>

namespace Hazel {

	Application* Application::s_Instance = nullptr;
	static std::thread::id s_MainThreadID;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification),m_RenderThread(ThreadingPolicy::SingleThreaded)
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		s_MainThreadID = std::this_thread::get_id();
		m_RenderThread.Run();  

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(WindowProps(m_Specification.Name));  // 这里创建了GLFW窗口、也初始化了RenderContext和SwapChain
		
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));
		HZ_CORE_ASSERT(NFD::Init() == NFD_OKAY);

		Renderer::Init();  // 把渲染API准备好，并提前创建了一些资源
		m_RenderThread.Pump();  // 因为前边的一些渲染相关的函数都是Render::Submit的，需要让渲染线程执行完他们

		// ImGui初始化
		//m_ImGuiLayer = new ImGuiLayer();
		//PushOverlay(m_ImGuiLayer);

		// TODO:其他系统初始化也在这里



	}

	Application::~Application()
	{
		HZ_PROFILE_FUNCTION();

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

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) 
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		HZ_PROFILE_FUNCTION();

		while (m_Running)
		{
			m_RenderThread.BlockUntilRenderComplete();  // 阻塞等待渲染线程完成渲染

			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime; 
			m_LastFrameTime = time;

			ExecuteMainThreadQueue();
			m_RenderThread.NextFrame();

			// Start rendering previous frame
			m_RenderThread.Kick(); // 通知渲染线程开始渲染（RenderCommandQueue缓存的命令全部执行）

			if (!m_Minimized)
			{
				// 重置DrawCall=0，重置描述符池,清空命令缓冲区、获取下一帧图片索引
				Renderer::RT_BeginFrame();  // 也是RT_ 只是没标明

				// 更新各层
				{
					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);  // 这里就是填装渲染指令的地方
				}

				// GUI渲染
				Application* app = this;
				if (m_Specification.EnableImGui)
				{
					Renderer::Submit([app]() { app->RenderImGui(); });
					Renderer::Submit([=]() { m_ImGuiLayer->End(); });
				}

				// 提交命令缓冲区、呈现图片
				Renderer::RT_EndFrame();  

			}

			// 记录信息
			m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;
			static uint64_t frameCounter = 0;
			frameCounter++;

			m_Window->OnUpdate();
		}
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
