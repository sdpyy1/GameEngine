#include "pch.h"
#include "Application.h"

#include <glad/glad.h>
#include "Engine/Input.h"
namespace Engine {

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		ENGINE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		// 通过不同系统的窗口API创建GUI窗口
		m_Window = std::unique_ptr<Window>(Window::Create());
		// 绑定窗口产生的事件发送到Application::OnEvent
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
		// 直接创建ImGui层并压入栈顶
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		// 事件分发器
		EventDispatcher dispatcher(e);

		// 关窗事件直接处理
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

		// 从栈顶开始向下传递事件
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			// 某一层进行事件处理
			(*--it)->OnEvent(e);
			// 如果事件被某一层处理掉了，就停止传递
			if (e.Handled) break;
		}

	}
	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
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
	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			for (Layer* layer : m_LayerStack) layer->OnUpdate();
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();
			m_Window->OnUpdate();
		}
	}
}

