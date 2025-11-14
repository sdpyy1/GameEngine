#pragma once

#include "Hazel/Core/Base.h"

#include "Hazel/Core/Window.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/core/RenderThread.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Platform/Windows/WindowsWindow.h"

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
		bool EnableImGui = true; // TODO����������ΪFalse�ᱨ��
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
		static void SetViewportSize(float width, float height){
			Get().m_ViewportWidth = width; Get().m_ViewportHeight = height;
		}
		static float GetViewportWidth() { return Get().m_ViewportWidth; }
		static float GetViewportHeight() { return Get().m_ViewportHeight; }
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }
		bool isMinimized() { return m_Minimized; }
		Ref<RenderContext> GetRenderContext() { return GetWindow()->GetRenderContext(); }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		void SubmitToMainThread(const std::function<void()>& function);
		static bool isRenderImGUI() {
			return Get().GetSpecification().EnableImGui;
		}
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		Ref<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		RenderThread m_RenderThread;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static Application* s_Instance;
		uint32_t m_CurrentFrameIndex = 0;
		float m_ViewportWidth = 1200.0f, m_ViewportHeight = 720.0f;
	};

	// To be defined in CLIENT
	Application* CreateApplication(ApplicationCommandLineArgs args);

}
