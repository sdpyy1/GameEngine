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

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();

		void SaveSceneAs();

		// 各种窗口创建
		void ViewportGUI();
		void DrawGizmo();
		void SettingGUI();
		void DebugTexture();
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		std::pair<float, float> GetMouseViewportSpace();
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);
	private:
		Ref<Scene> m_Scene;  // 场景
		EditorCamera m_EditorCamera; // 摄像机 TODO:交给场景管理
		// Gizmo's
		int m_GizmoType = -1;
		Entity m_HoveredEntity;
		Entity m_SelectedEntity;
		// 面板
		AssetManagerPanel m_AssetManagerPanel;
		FolderPreviewPanel m_FolderPreviewPanel;
		ImGuiLogPanel m_LogPanel;
		//状态
		ImVec2 m_ViewportBounds[2] = { {0,0},{1216,849} };
		bool isMouseInViewport = false;
		bool firstRenderGUI = true;
		std::string m_SceneFilePath;
		struct SelectionData
		{
			Entity Entity;
			Submesh* submesh = nullptr;
			Ref<MeshSource> meshSource = nullptr;
			float Distance = 0.0f;
		};
	};
}
