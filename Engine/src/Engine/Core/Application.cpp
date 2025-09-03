#include "pch.h"
#include "Application.h"

#include <glad/glad.h>
#include "Engine/Core/Input.h"
#include <GLFW/glfw3.h>
#include "Engine/Renderer/Renderer.h"
namespace Engine {

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name)

	{
		ENGINE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		// 通过不同系统的窗口API创建GUI窗口
		m_Window = Window::Create(WindowProps(name));
		// 绑定窗口产生的事件发送到Application::OnEvent
		m_Window->SetEventCallback(ENGINE_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

		// 直接创建ImGui层并压入栈顶
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		Renderer::Shutdown();
	}


	void Application::OnEvent(Event& e)
	{
		// 事件分发器
		EventDispatcher dispatcher(e);

		// 关窗事件直接处理
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));


		// 从栈顶开始向下传递事件
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			// 某一层进行事件处理
			(*it)->OnEvent(e);
			// 如果事件被某一层处理掉了，就停止传递
			if (e.Handled) break;
		}

	}
	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}
	void Application::Close()
	{
		m_Running = false;
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}
	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}
	void Application::Run()
	{
		while (m_Running)
		{
			// 计算一帧使用的时间
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);
			}

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}
}

