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

		// 1. 配置停靠容器窗口的标志：NoDocking 表示「自身不允许被停靠」，但内部可创建停靠空间
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus; // 避免抢占焦点

		// 2. 让停靠容器铺满整个主视口（屏幕）
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);       // 位置 = 屏幕左上角
		ImGui::SetNextWindowSize(viewport->Size);     // 大小 = 屏幕大小
		ImGui::SetNextWindowViewport(viewport->ID);   // 绑定到主视口

		// 3. 去除容器窗口的装饰（圆角、边框），让它成为「隐形容器」
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		// 4. 开始绘制停靠容器窗口
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		// ！！！关键：在容器内部创建停靠空间（唯一 ID 用于标识该停靠区域）
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace"); // 给停靠空间起一个唯一 ID
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);   // 0 表示默认停靠方向（上下左右都支持）
		ImGui::End(); // 结束容器窗口绘制

		ImGui::PopStyleVar(2); // 恢复之前的样式（WindowRounding 和 WindowBorderSize）

		// 5. 现在 ViewPort 和 Setting 窗口可以停靠到上面的 DockSpace 了
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
