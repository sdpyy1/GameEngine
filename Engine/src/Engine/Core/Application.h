#pragma once
#include "Core.h"

#include "Engine/Events/Event.h"
#include "Window.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Core/LayerStack.h"
#include "Engine/Core/Timestep.h"

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
		bool OnWindowResize(WindowResizeEvent& e);


	private:
		// 关窗事件处理函数
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		// 通过不同系统的窗口API创建的GUI窗口
		std::unique_ptr<Window> m_Window;
		// 运行状态
		bool m_Running = true;
		// 层栈
		LayerStack m_LayerStack;
		// ImGui层
		ImGuiLayer* m_ImGuiLayer;

		// 帧时间
		float m_LastFrameTime = 0.0f;
		bool m_Minimized = false;


	private:
		static Application* s_Instance;
	};
}


