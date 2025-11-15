#include "hzpch.h"
//
//#include "EditorLayer.h"
//#include "Hazel/Scene/SceneSerializer.h"
//#include "Hazel/Math/Math.h"
//#include "ImGuizmo.h"
//#include <Hazel/Asset/Model/Mesh.h>
//#include "Hazel/Math/Ray.h"
//#include "Hazel/Utils/FileSystem.h"
//#include "Hazel/Core/Input.h"
//#include <glm/gtc/type_ptr.hpp>
//namespace Hazel {
//	EditorLayer::EditorLayer()
//		: Layer("EditorLayer"), m_EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f)
//	{
//
//	}
//
//	void EditorLayer::OnAttach()
//	{
//	}
//
//	void EditorLayer::OnUpdate(Timestep ts)
//	{
//	}
//
//	void EditorLayer::OnImGuiRender()
//	{
//		
//	}
//
//	void EditorLayer::OnEvent(Event& e)
//	{
//		EventDispatcher dispatcher(e);
//
//		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) { return OnMouseButtonPressed(event); });
//	}
//
//}
