#include "EditorLayer.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Renderer/Font.h"

#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ImGuizmo.h"
#include <GLFW/include/GLFW/glfw3.h>
#include <Hazel/Asset/AssetMetadata.h>
#include <Hazel/Asset/AssetImporter.h>
#include <Hazel/Asset/Model/Mesh.h>
#include "Hazel/Scene/SceneRender.h"

namespace Hazel {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f)
	{
		m_Scene = Ref<Scene>::Create();
		m_AssetManagerPanel.SetContext(m_Scene);
		m_SceneRender = Ref<SceneRender>::Create();
		m_SelectedEntity = Entity(); // 初始无选中实体
		m_GizmoType = -1;           // 初始无 Gizmo
	}

	void EditorLayer::OnAttach()
	{
		// 模型加载
		AssetMetadata metadata;
		//metadata.FilePath = "assets/model/helmet_pbr/DamagedHelmet.gltf";
		//metadata.Type = AssetType::MeshSource;
		//Ref<Asset> Helmet;
		//AssetImporter::TryLoadData(metadata, Helmet);
		//Entity helmet = m_Scene->CreateEntity("Helmet");
		//helmet.AddComponent<StaticMeshComponent>(Helmet->Handle);

		metadata.FilePath = "assets/model/m1911/m1911.gltf";
		metadata.Type = AssetType::MeshSource;
		Ref<Asset> eagle;
		AssetImporter::TryLoadData(metadata, eagle);	
		Entity gun = m_Scene->CreateEntity("Eagle");
		gun.AddComponent<StaticMeshComponent>(eagle->Handle);
		gun.GetComponent<TransformComponent>().SetTranslation(glm::vec3(0.0f, 2.0f, 0.0f));
		gun.GetComponent<TransformComponent>().SetUniformScale(15);
		Ref<MeshSource> ms = eagle.As<MeshSource>();
		ms->GetAnimation("Fire", *ms->GetSkeleton(), false, glm::vec3(1), 0);
		m_HoveredEntity = gun;
		m_SelectedEntity = gun;
	}

	void EditorLayer::OnDetach() {}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_EditorCamera.SetActive(true);
		m_EditorCamera.OnUpdate(ts, isMouseInViewport);
		m_Scene->OnEditorRender(m_SceneRender, m_EditorCamera);
	}

	void EditorLayer::OnImGuiRender()
	{
		// Dockspace 背景窗口
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		// 覆盖GLFW窗口的Main窗口
		ImGui::Begin("Main Window", nullptr, window_flags);
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);

		ImGui::End();
		ImGui::PopStyleVar(2);

		// 其他窗口绘制
		ViewportGUI();
		TestGUI();
		if (m_AssetManagerPanel.isOpen) {
			m_AssetManagerPanel.OnImGuiRender();
		}
	}

	void EditorLayer::OnEvent(Event& e)
	{
	}

	void EditorLayer::ViewportGUI()
	{
		ImGui::Begin("Viewport");
		m_ViewportBounds[0] = ImGui::GetWindowPos();
		// 程序中ViewportSize都来自这里
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportBounds[1] = ImVec2(m_ViewportBounds[0].x + viewportSize.x, m_ViewportBounds[0].y + viewportSize.y);
		m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);
		m_Scene->SetViewprotSize(viewportSize.x, viewportSize.y);
		m_SceneRender->SetViewprotSize(viewportSize.x, viewportSize.y);
		Application::SetViewportSize(viewportSize.x, viewportSize.y);
		
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		isMouseInViewport =
			mousePos.x >= m_ViewportBounds[0].x && mousePos.x <= m_ViewportBounds[1].x &&
			mousePos.y >= m_ViewportBounds[0].y && mousePos.y <= m_ViewportBounds[1].y;
		m_Scene->OutputRenderRes(m_SceneRender);
		DrawGizmo();
		ImGui::End();
	}

	void EditorLayer::DrawGizmo()
	{
		m_SelectedEntity = m_AssetManagerPanel.GetSelectedEntity();
		bool rightMouseDown = ImGui::IsMouseDown(Mouse::ButtonRight);
		if (rightMouseDown) {
			m_GizmoType = -1;
		}
		else if(!ImGuizmo::IsUsing()) {
			if (Input::IsKeyDown(Key::W)) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			if (Input::IsKeyDown(Key::E)) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			if (Input::IsKeyDown(Key::R)) m_GizmoType = ImGuizmo::OPERATION::SCALE;
			if (Input::IsKeyDown(Key::Q)) m_GizmoType = -1;
		}
		if (!m_SelectedEntity || m_GizmoType == -1)
			return;

		if (!isMouseInViewport)
			return;
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist(); 
		float x = m_ViewportBounds[0].x;
		float y = m_ViewportBounds[0].y;
		float width = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
		float height = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
		ImGuizmo::SetRect(x, y, width, height);
		const glm::mat4& cameraProjection = m_EditorCamera.GetProjectionMatrix();
		glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();
		auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
		glm::mat4 transform = tc.GetTransform();
		bool snap = Input::IsKeyDown(Key::LeftControl);
		float snapValue = (m_GizmoType == ImGuizmo::ROTATE) ? 45.0f : 0.5f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		ImGuizmo::Manipulate(glm::value_ptr(cameraView),
			glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_GizmoType,
			ImGuizmo::LOCAL,
			glm::value_ptr(transform),
			nullptr,
			snap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			glm::vec3 translation, rotation, scale;
			Math::DecomposeTransform(transform, translation, rotation, scale);

			glm::vec3 deltaRotation = rotation - tc.Rotation;
			tc.Translation = translation;
			tc.Rotation += deltaRotation;
			tc.Scale = scale;
		}
	}

	void EditorLayer::TestGUI()
	{
		ImGui::Begin("Setting");
		ImGui::Text("Some Settings...");
		ImGui::End();
	}
}
