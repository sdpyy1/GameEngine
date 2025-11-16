#pragma once
#include "Hazel/Events/MouseEvent.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/FolderPreviewPanel.h"
#include "Panels/LogPanel.h"
#include <Hazel/Asset/Model/Mesh.h>
namespace Hazel {
	class SubMesh;
	class ImGuiRendererManager
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;
		void SetDarkThemeV2Colors();
		void Tick(float deltaTime);
		void ImGuiCommand();
		bool OnEvent(Event& e);
		// 各种窗口创建
		void ViewportGUI();
		void DrawGizmo();
		void SettingGUI();
		void DebugTexture();
		static std::shared_ptr<ImGuiRendererManager> Create();
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		std::pair<float, float> GetMouseViewportSpace(); // NDC坐标
		std::pair<glm::vec3, glm::vec3> CastRay(EditorCamera& camera, float mx, float my);
		void pre();
		void SetScene(std::shared_ptr<Scene> activeScene);
	private:

		// Gizmo's
		int m_GizmoType = -1;
		Entity m_HoveredEntity;
		Entity m_SelectedEntity;
		// 面板
		AssetManagerPanel m_AssetManagerPanel;
		FolderPreviewPanel m_FolderPreviewPanel{ "assets" };
		ImGuiLogPanel m_LogPanel;
		//状态
		ImVec2 m_ViewportBounds[2] = { {0,0},{1216,849} };
		bool isMouseInViewport = false;
		bool firstRenderGUI = true;
		struct SelectionData
		{
			Entity Entity;
			Submesh* submesh = nullptr;
			Ref<MeshSource> meshSource = nullptr;
			float Distance = 0.0f;
		};
	};

}
