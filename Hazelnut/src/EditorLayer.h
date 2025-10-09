#pragma once

#include "Hazel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Hazel/Renderer/EditorCamera.h"

namespace Hazel {
	// 编辑器层
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;


		// 各种窗口创建
		void ViewportGUI();
		void TestGUI();
	private:
		Scene scene;  // 场景
		EditorCamera m_EditorCamera; // 摄像机
	};

}
