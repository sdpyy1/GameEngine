#pragma once
// 存储需要对外暴漏的头文件, sandbox只要包含Engine.h就可以使用这些功能
#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/KeyCodes.h"
#include "Engine/Core/MouseButtonCodes.h"
#include "Engine/Core/Layer.h"
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
#include "Engine/Renderer/OrthographicCameraController.h"
#include "Engine/Renderer/Renderer2D.h"


// -----------------------------------
namespace Engine
{
	// to be defined in CLIENT
	Application* CreateApplication();
}

// -----Entry Point-----------------
//#include "Engine/Core/EntryPoint.h"   -- 写在这里会导致Sandbox中的cpp使用后重复包含main函数
// ---------------------------------