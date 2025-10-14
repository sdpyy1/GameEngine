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
		: Layer("EditorLayer"), m_EditorCamera(45.0f, 1600.0f, 1200.0f, 0.1f, 1000.0f)
	{
		m_SceneRender = Ref<SceneRender>::Create();
	}

	void EditorLayer::OnAttach()
	{
		// 模型加载
		AssetMetadata metadata;
		//metadata.FilePath = "D:/Hazel-3D-2023/Hazelnut/Resources/Meshes/Default/Capsule.gltf";
		//metadata.FilePath = "assets/model/helmet_pbr/DamagedHelmet.gltf";
		metadata.FilePath = "assets/model/desert-eagle/scene.gltf";
		metadata.Type = AssetType::MeshSource;
		Ref<Asset> Helmet;
		AssetImporter::TryLoadData(metadata, Helmet);
		Entity a = scene.CreateEntity("aaa");
		a.AddComponent<StaticMeshComponent>(Helmet->Handle);
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_EditorCamera.SetActive(true);
		m_EditorCamera.OnUpdate(ts);

		scene.OnEditorRender(m_SceneRender,m_EditorCamera);
	}

	void EditorLayer::OnImGuiRender()
	{
		// ImGui + Dockspace Setup ------------------------------------------------------------------------------
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		// 设置是否允许通过托角来ReSize
		io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;
		//创建背景窗口，它要设置不可Dock
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		// 背景窗口贴死GLFW窗口
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos); 
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		//去除背景窗口的装饰（圆角、边框），让它成为「隐形容器」
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		//  开始绘制停靠容器窗口
		ImGui::Begin("Main Window", nullptr, window_flags);
		// 在容器内部创建停靠空间（唯一 ID 用于标识该停靠区域）
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace"); 
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);

		// 窗口绘制在这里写
		ViewportGUI();
		TestGUI();
		ImGui::End();
		ImGui::PopStyleVar(2); 
	}

	void EditorLayer::OnEvent(Event& e)
	{

	}

	void EditorLayer::ViewportGUI()
	{
		ImGui::Begin("ViewPort");
		scene.OutputRenderRes(m_SceneRender);
		ImGui::End();
	}
	void EditorLayer::TestGUI()
	{
		ImGui::Begin("Setting");
		ImGui::Text("Im some Settings");
		ImGui::End();
	}
}
