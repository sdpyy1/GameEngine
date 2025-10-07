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
#include <Platform/Vulkan/VulkanImage.h>
#include "imgui/examples/imgui_impl_vulkan.h"
#include <GLFW/include/GLFW/glfw3.h>

namespace Hazel {


	EditorLayer::EditorLayer()
		: Layer("EditorLayer"),camera(60.0f, 16.0f / 9.0f, 0.1f, 10000.0f)
	{
	}

	void EditorLayer::OnAttach()
	{
		scene.CreateEntity("aaa");
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		scene.RenderVukan();
	}

	void EditorLayer::OnImGuiRender()
	{
		// ImGui + Dockspace Setup ------------------------------------------------------------------------------
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;

		// 1. ����ͣ���������ڵı�־��NoDocking ��ʾ����������ͣ���������ڲ��ɴ���ͣ���ռ�
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus; // ������ռ����

		// 2. ��ͣ�����������������ӿڣ���Ļ��
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);       // λ�� = ��Ļ���Ͻ�
		ImGui::SetNextWindowSize(viewport->Size);     // ��С = ��Ļ��С
		ImGui::SetNextWindowViewport(viewport->ID);   // �󶨵����ӿ�

		// 3. ȥ���������ڵ�װ�Σ�Բ�ǡ��߿򣩣�������Ϊ������������
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		// 4. ��ʼ����ͣ����������
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		// �������ؼ����������ڲ�����ͣ���ռ䣨Ψһ ID ���ڱ�ʶ��ͣ������
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace"); // ��ͣ���ռ���һ��Ψһ ID
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);   // 0 ��ʾĬ��ͣ�������������Ҷ�֧�֣�
		ImGui::End(); // �����������ڻ���

		ImGui::PopStyleVar(2); // �ָ�֮ǰ����ʽ��WindowRounding �� WindowBorderSize��

		// 5. ���� ViewPort �� Setting ���ڿ���ͣ��������� DockSpace ��
		ImGui::Begin("ViewPort");
		scene.SetViewPortImage();
		ImGui::End();

		ImGui::Begin("Setting");
		ImGui::Text("Im some Settings");
		ImGui::End();

	}

	void EditorLayer::OnEvent(Event& e)
	{

	}

}
