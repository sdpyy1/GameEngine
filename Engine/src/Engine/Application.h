#pragma once
#include "Core.h"

#include "Engine/Events/Event.h"
#include "Window.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/LayerStack.h"

#include "Engine/ImGui/ImGuiLayer.h"

namespace Engine {
	class ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
		void OnEvent(Event& e);   // 窗口事件回调函数
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		inline Window& GetWindow() { return *m_Window; }
		inline static Application& Get() { return *s_Instance; }
	private:
		// 关窗事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);
		// 通过不同系统的窗口API创建的GUI窗口
		std::unique_ptr<Window> m_Window;
		// 运行状态
		bool m_Running = true;
		// 层栈
		LayerStack m_LayerStack;
		// ImGui层
		ImGuiLayer* m_ImGuiLayer;

	private:
		static Application* s_Instance;
	};
}


