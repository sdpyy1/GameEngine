#pragma once
// 存储需要对外暴漏的头文件, sandbox只要包含Engine.h就可以使用这些功能
#include "Engine/Application.h"
#include "Engine/Log.h"
#include "Engine/Input.h"
#include "Engine/KeyCodes.h"
#include "Engine/MouseButtonCodes.h"
#include "Engine/Layer.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "imgui.h"
#include "Engine/Core/Timestep.h"


// ---Renderer------------------------
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/OrthographicCamera.h"
// -----------------------------------
namespace Engine
{
	// to be defined in CLIENT
	Application* CreateApplication();
}

// -----Entry Point-----------------
#include "Engine/EntryPoint.h"
// ---------------------------------