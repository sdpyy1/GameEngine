#include "EditorLayer.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "Hazel/Math/Math.h"
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
#include <random>

namespace Hazel {
	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f), m_FolderPreviewPanel("assets")
	{
		m_Scene = Ref<Scene>::Create();
		m_AssetManagerPanel.SetContext(m_Scene);
		m_FolderPreviewPanel.SetContext(m_Scene);
		m_SelectedEntity = {};
		m_GizmoType = -1;
	}

	void EditorLayer::OnAttach()
	{
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_EditorCamera.OnUpdate(ts, isMouseInViewport);
		m_Scene->OnEditorRender(ts, m_EditorCamera);
	}

	void EditorLayer::OnImGuiRender()
	{
		// ========== Dockspace 主窗口 ==========
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_MenuBar; // 为菜单栏预留空间

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::Begin("Main Window", nullptr, window_flags);

		// ========== 顶部菜单栏 ==========
		if (ImGui::BeginMenuBar())
		{
			// --- File 菜单 ---
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
				{
					m_Scene = Ref<Scene>::Create();
					m_AssetManagerPanel.SetContext(m_Scene);
					m_FolderPreviewPanel.SetContext(m_Scene);
				}

				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
				{
					OpenScene();
				}

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
				{
					SaveScene();
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					Application::Get().Close();

				ImGui::EndMenu();
			}

			// --- View 菜单 ---
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Folder Preview", nullptr, &m_FolderPreviewPanel.isOpen);
				ImGui::MenuItem("Asset Manager", nullptr, &m_AssetManagerPanel.isOpen);
				ImGui::EndMenu();
			}

			// --- Help 菜单 ---
			if (ImGui::BeginMenu("Help"))
			{
				ImGui::Text("Hazel Editor - Custom Build");
				ImGui::Separator();
				ImGui::Text("Ctrl+S  Save Scene");
				ImGui::Text("W/E/R   Gizmo Control");
				ImGui::Text("Right Click  Free Look");
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// ========== DockSpace 区域 ==========
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);
		ImGui::End(); // Main Window
		ImGui::PopStyleVar(2);

		// ========== 其他面板绘制 ==========
		ViewportGUI();
		SettingGUI();

		if (m_FolderPreviewPanel.isOpen)
			m_FolderPreviewPanel.OnImGuiRender();

		if (m_AssetManagerPanel.isOpen)
			m_AssetManagerPanel.OnImGuiRender();

		if(m_LogPanel.isOpen)
            m_LogPanel.OnImGuiRender();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) { return OnMouseButtonPressed(event); });

	}
	bool EditorLayer::OpenScene()
	{
		std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "Hazel Scene", "hscene" } });
		if (!filepath.empty())
			return OpenScene(filepath);

		return false;
	}
	bool EditorLayer::OpenScene(const std::filesystem::path& filepath)
	{
		if (filepath.extension() != ".hscene")
		{
			return false;
		}
		if (!FileSystem::Exists(filepath))
		{
			HZ_CORE_ERROR("Tried loading a non-existing scene: {0}", filepath);
			return false;
		}

		if (m_Scene) {
			m_Scene->ClearEntities();
			m_AssetManagerPanel.ClearState();
		}
		SceneSerializer serializer(m_Scene);
		serializer.Deserialize(filepath.string());
		m_SceneFilePath = filepath.string();
		std::replace(m_SceneFilePath.begin(), m_SceneFilePath.end(), '\\', '/');
		return true;
	}

	void EditorLayer::SaveScene()
	{
		if (!m_SceneFilePath.empty())
		{
			SceneSerializer serializer(m_Scene);
			serializer.Serialize(m_SceneFilePath);
		}
		else
		{
			SaveSceneAs();
		}
	}

	void EditorLayer::SaveSceneAs()
	{
		std::filesystem::path filepath = FileSystem::SaveFileDialog({ { "Hazel Scene (*.hscene)", "hscene" } });

		if (filepath.empty())
			return;

		if (!filepath.has_extension())
			filepath += SceneSerializer::DefaultExtension;

		SceneSerializer serializer(m_Scene);
		serializer.Serialize(filepath.string());

		std::filesystem::path path = filepath;
		m_SceneFilePath = filepath.string();
		std::replace(m_SceneFilePath.begin(), m_SceneFilePath.end(), '\\', '/');
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
		//m_SceneRender->SetViewprotSize(viewportSize.x, viewportSize.y);
		Application::SetViewportSize(viewportSize.x, viewportSize.y);

		ImVec2 mousePos = ImGui::GetIO().MousePos;
		isMouseInViewport =
			mousePos.x >= m_ViewportBounds[0].x && mousePos.x <= m_ViewportBounds[1].x &&
			mousePos.y >= m_ViewportBounds[0].y && mousePos.y <= m_ViewportBounds[1].y;
		m_Scene->OutputViewport();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				// 验证 payload 数据有效性（确保是字符串）
				IM_ASSERT(payload->DataSize > 0);
				const char* droppedPath = (const char*)payload->Data;
				OpenScene(droppedPath);
			}
			ImGui::EndDragDropTarget();
		}
		DrawGizmo();

		ImGui::End();
	}

	void EditorLayer::DrawGizmo()
	{
		m_SelectedEntity = m_AssetManagerPanel.GetSelectedEntity();
		bool rightMouseDown = ImGui::IsMouseDown((int)Button::Right);
		if (rightMouseDown) {
			m_GizmoType = -1;
		}
		else if (!ImGuizmo::IsUsing()) {
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

			glm::vec3 deltaRotation = rotation - tc.GetRotationEuler();
			tc.Translation = translation;
			tc.SetRotation(tc.GetRotationEuler() += deltaRotation);
			tc.Scale = scale;
		}
	}

	void EditorLayer::SettingGUI()
	{
		ImGui::Begin("Setting");

		// 阴影设置栏目
		if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 阴影类型选择
			ImGui::RadioButton("Hard Shadow", &m_Scene->GetRenderSettingData().ShadowType, 0);
			ImGui::RadioButton("PCF", &m_Scene->GetRenderSettingData().ShadowType, 1);
			ImGui::RadioButton("PCSS", &m_Scene->GetRenderSettingData().ShadowType, 2);
			ImGui::Checkbox("Show Cascade", &m_Scene->GetRenderSettingData().deBugCSM);
		}

		ImGui::End();
	}

	void EditorLayer::OnDetach() {}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (event.GetMouseButton() != Mouse::ButtonLeft)
			return false;
		if (ImGuizmo::IsOver())
			return false;
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f) {
			std::vector<SelectionData> selectionData;

			auto [origin, direction] = CastRay(m_EditorCamera, mouseX, mouseY);
			auto meshEntities = m_Scene->GetAllEntitiesWith<StaticMeshComponent>();
			for (auto e : meshEntities) {
				Entity entity = { e, m_Scene.Raw() };
				auto& mc = entity.GetComponent<StaticMeshComponent>();
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mc.StaticMesh);
				auto& submeshes = meshSource->GetSubmeshes();
				for (uint32_t i = 0; i < submeshes.size(); i++)
				{
					auto& submesh = submeshes[i];
					glm::mat4 transform = m_Scene->GetWorldSpaceTransformMatrix(entity);
					Ray ray = {
						glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
						glm::inverse(glm::mat3(transform * submesh.Transform)) * direction
					};

					float t;
					bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
					if (intersects)
					{
						const auto& triangleCache = meshSource->GetTriangleCache(i);
						for (const auto& triangle : triangleCache)
						{
							if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
							{
								selectionData.push_back({ entity, &submesh,meshSource, t });
								break;
							}
						}
					}
				}
			}
			auto dynamicMeshEntities = m_Scene->GetAllEntitiesWith<DynamicMeshComponent>();

			for (auto e : dynamicMeshEntities) {
				Entity entity = { e, m_Scene.Raw() };
				auto& mc = entity.GetComponent<DynamicMeshComponent>();
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mc.meshSource);
				auto& submeshes = meshSource->GetSubmeshes();
				for (uint32_t i = 0; i < submeshes.size(); i++)
				{
					auto& submesh = submeshes[i];
					glm::mat4 transform = m_Scene->GetWorldSpaceTransformMatrix(entity);
					Ray ray = {
						glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
						glm::inverse(glm::mat3(transform * submesh.Transform)) * direction
					};

					float t;
					bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
					if (intersects)
					{
						const auto& triangleCache = meshSource->GetTriangleCache(i);
						for (const auto& triangle : triangleCache)
						{
							if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
							{
								selectionData.push_back({ entity, &submesh,meshSource, t });
								break;
							}
						}
					}
				}
			}
			if (selectionData.size() > 0) {
				std::sort(selectionData.begin(), selectionData.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
				HZ_CORE_TRACE("Selected Entity: {}", selectionData[0].Entity.GetComponent<TagComponent>().Tag);
				m_SelectedEntity = selectionData[0].Entity;
				m_AssetManagerPanel.SetSelectedEntity(selectionData[0].Entity);
			}

		}

		return false;
	}
	std::pair<float, float> EditorLayer::GetMouseViewportSpace()  // NDC坐标
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;
		mx -= viewportBounds[0].x;
		my -= viewportBounds[0].y;
		auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
		auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}
	std::pair<glm::vec3, glm::vec3> EditorLayer::CastRay(const EditorCamera& camera, float mx, float my)
	{
		glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		auto inverseProj = glm::inverse(camera.GetProjectionMatrix());
		auto inverseView = glm::inverse(glm::mat3(camera.GetViewMatrix()));

		glm::vec4 ray = inverseProj * mouseClipPos;
		glm::vec3 rayPos = camera.GetPosition();
		glm::vec3 rayDir = inverseView * glm::vec3(ray);

		return { rayPos, rayDir };
	}
}
