#pragma once

#include "Hazel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Hazel/Renderer/EditorCamera.h"

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
		void TestGUI();
	private:
		Scene scene;  // ����
		EditorCamera m_EditorCamera; // �����
	};

}
