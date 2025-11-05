#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_FE
// For use by Hazel applications
#define HZ_VERSION "2025.10.28"

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Core/Assert.h"

#include "Hazel/Core/Timestep.h"
#include "Hazel/Math/ray.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"

#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/ImGui/ImGuiLogPanel.h"
#include "Hazel/Utils/FileSystem.h"

#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/ScriptableEntity.h"
#include "Hazel/Scene/Components.h"

// ---Renderer------------------------
#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"
// -----------------------------------
