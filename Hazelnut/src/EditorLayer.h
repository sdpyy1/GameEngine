#pragma once

#include "Hazel.h"

#include "Hazel/Renderer/EditorCamera.h"
#include "../AssetManagerPanel.h"
#include "imgui.h"
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
		Ref<Scene> m_Scene;  // ����
		Ref<SceneRender> m_SceneRender; // ������Ⱦ��
		EditorCamera m_EditorCamera; // �����


		// ���
		AssetManagerPanel m_AssetManagerPanel;


		//״̬
		ImVec2 m_ViewportBounds[2];
		bool isMouseInViewport = false;
	};

}
