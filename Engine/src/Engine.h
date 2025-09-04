#pragma once
// 存储需要对外暴漏的头文件, sandbox只要包含Engine.h就可以使用这些功能
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"
#include "Hazel/Core/Layer.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "imgui/imgui.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Debug/Instrumentor.h"
#include "Hazel/Renderer/Framebuffer.h"
#include "Hazel/Scene/Entity.h"

#include "Hazel/Scene/ScriptableEntity.h"

#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Components.h"

// ---Renderer------------------------
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderCommand.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/OrthographicCamera.h"
#include "Hazel/Renderer/OrthographicCameraController.h"
#include "Hazel/Renderer/Renderer2D.h"


// -----------------------------------
namespace Hazel
{
	// to be defined in CLIENT
	Application* CreateApplication();
}

// -----Entry Point-----------------
//#include "Hazel/Core/EntryPoint.h"   -- 写在这里会导致Sandbox中的cpp使用后重复包含main函数
// ---------------------------------