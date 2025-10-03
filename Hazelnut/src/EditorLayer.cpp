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
	}

	void EditorLayer::OnEvent(Event& e)
	{

	}

}
