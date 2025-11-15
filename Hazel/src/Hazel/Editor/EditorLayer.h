//#pragma once
//#include "Hazel/Renderer/EditorCamera.h"
//#include "Panels/AssetManagerPanel.h"
//#include "imgui.h"
//#include "ImGuizmo.h"
//#include "Panels/FolderPreviewPanel.h"
//#include "Panels/LogPanel.h"
//
//namespace Hazel {
//	// 编辑器层
//	class EditorLayer : public Layer
//	{
//	public:
//		EditorLayer();
//		virtual ~EditorLayer() = default;
//
//		void OnAttach() override;
//		void OnDetach() override;
//
//		void OnUpdate(Timestep ts) override;
//		void OnImGuiRender() override;
//		void OnEvent(Event& e) override;
//		bool OpenScene();
//		bool OpenScene(const std::filesystem::path& filepath);
//		void SaveScene();
//
//		void SaveSceneAs();
//
//		// 各种窗口创建
//		void ViewportGUI();
//		void DrawGizmo();
//		void SettingGUI();
//		void DebugTexture();
//		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
//		std::pair<float, float> GetMouseViewportSpace();
//		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);
//	private:
//		Ref<Scene> m_Scene;  // 场景
//		EditorCamera m_EditorCamera; // 摄像机 TODO:交给场景管理
//		
//	};
//}
