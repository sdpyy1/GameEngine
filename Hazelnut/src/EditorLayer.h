#pragma once

#include "Hazel.h"

#include "Hazel/Renderer/EditorCamera.h"
#include "Panels/AssetManagerPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "Panels/FolderPreviewPanel.h"

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
		void DrawGizmo();
		void TestGUI();
	private:
		Ref<Scene> m_Scene;  // 场景
		EditorCamera m_EditorCamera; // 摄像机（目前设计摄像机不归场景管理）
		// Gizmo
		int m_GizmoType = -1;
		Entity m_HoveredEntity;
		Entity m_SelectedEntity;
		// 面板
		AssetManagerPanel m_AssetManagerPanel;
		FolderPreviewPanel m_FolderPreviewPanel;

		//状态
		ImVec2 m_ViewportBounds[2] = { {0,0},{1216,849} };
		bool isMouseInViewport = false;
		bool firstRenderGUI = true;
	};

}
