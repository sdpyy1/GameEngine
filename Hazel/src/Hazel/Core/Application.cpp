#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Editor/EditorLayer.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "Hazel/Asset/AssetImporter.h"
#include <nfd.hpp>

namespace Hazel {
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{

		ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(WindowProps(m_Specification.Name, 1950, 1300)); 
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));


		NFD::Init();
		m_RendererManager = std::make_shared<RendererManager>();
		AssetImporter::Init();

	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_RendererManager->ExecutePreFrame();

			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;


			m_RendererManager->tick(timestep,m_Minimized);
			m_Window->tick();

			// 主线程 FrameIndex 更新
			m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;
		}
	}

	Application::~Application()
	{
		Renderer::Shutdown();
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

		for (auto it = m_RendererManager->GetLayerStatck().rbegin(); it != m_RendererManager->GetLayerStatck().rend(); ++it)
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
}
