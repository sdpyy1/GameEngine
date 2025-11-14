#pragma once

#include "Hazel/Core/Base.h"

#include "Hazel/Core/Window.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Platform/Windows/WindowsWindow.h"
#include <Hazel/Renderer/RendererManager.h>

namespace Hazel {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			ASSERT(index < Count);
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Hazel Application";
		std::string WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
	};
	class VulkanContext;
	class Application
	{
	public:
		void Run();

		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void OnEvent(Event& e);

		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool isRunning() { return m_Running; }
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Ref<WindowsWindow>& GetWindow() { return m_Window.As<WindowsWindow>(); }
		void RenderImGui();

		void Close();

		static Application& Get() { return *s_Instance; }
		bool isMinimized() { return m_Minimized; }
		Ref<RenderContext> GetRenderContext() { return GetWindow()->GetRenderContext(); }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		ApplicationSpecification m_Specification;
		Ref<Window> m_Window;
		bool m_Running = true;
		bool m_Minimized = false;
		float m_LastFrameTime = 0.0f;

		std::shared_ptr<RendererManager> m_RendererManager;
	private:
		static Application* s_Instance;
		uint32_t m_CurrentFrameIndex = 0;
	};
}
