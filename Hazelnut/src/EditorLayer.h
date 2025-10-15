#pragma once

#include "Hazel.h"

#include "Hazel/Renderer/EditorCamera.h"
#include "Panels/AssetManagerPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Hazel {
	// �༭����
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


		// ���ִ��ڴ���
		void ViewportGUI();
		void DrawGizmo();
		void TestGUI();
	private:
		Ref<Scene> m_Scene;  // ����
		Ref<SceneRender> m_SceneRender; // ������Ⱦ��
		EditorCamera m_EditorCamera; // �����
		// Gizmo
		int m_GizmoType = -1;
		Entity m_HoveredEntity;
		Entity m_SelectedEntity;
		// ���
		AssetManagerPanel m_AssetManagerPanel;

		//״̬
		ImVec2 m_ViewportBounds[2] = { {0,0},{1600,1200} };
		bool isMouseInViewport = false;
	};

}
