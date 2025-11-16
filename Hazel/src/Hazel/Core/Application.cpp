#include "hzpch.h"

#include <nfd.hpp>
#include <GLFW/glfw3.h>
#include "Hazel/Core/Application.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Renderer/RendererManager.h>
#include "Hazel/Platform/Windows/WindowsWindow.h"
#include "Hazel/Core/Window.h"

namespace Hazel {
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{

		ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create(WindowProps(m_Specification.Name, 1950, 1300)); 
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));


		NFD::Init();
		m_SceneManager = std::make_shared<SceneManager>();
		m_RendererManager = std::make_shared<RendererManager>();
		AssetImporter::Init();

	}

	void Application::Tick()
	{
		while (m_Running)
		{
			m_RendererManager->ExecutePreFrame(); // RT_thread

			float timestep = GetTimePreFrame();

			m_SceneManager->Tick(timestep);

			if (!m_Minimized) {
				m_RendererManager->Tick(timestep);
			}

			m_Window->Tick();

			m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;
		}
	}
	float Application::GetTimePreFrame()
	{
		float time = glfwGetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return timestep;
	}


	void Application::Close()
	{
		m_Running = false;
	}


	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<WindowMinimizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowMinimize));
		if (m_RendererManager->OnEvent(e)) {
			return;
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
		RENDER_SUBMIT([&window, width, height]() mutable
			{
				window->GetSwapChain().OnResize(width, height);
			});

		return false;
	}

	Hazel::Ref<Hazel::WindowsWindow>& Application::GetWindow()
	{
		return m_Window.As<WindowsWindow>();
	}

	Hazel::Ref<Hazel::RenderContext> Application::GetRenderContext()
	{
		return GetWindow()->GetRenderContext();
	}

}
