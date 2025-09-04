include "./vendor/premake/premake_customization/solution_items.lua"

workspace "GameEngine"
	architecture "x86_64"
	startproject "Editor" 
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	flags
	{
		"MultiProcessorCompile"
	}
	-- 隐藏一些警告
	filter "system:windows"
        buildoptions { 
        "/wd4828", -- warning C4828: The file contains a character that cannot be represented in the current code page (936). Save the file in Unicode format to prevent data loss
        "/utf-8", -- Use UTF-8 as the source file encoding
		"/wd4005"   -- imgui.h(10): warning C4005: 'IMGUI_DISABLE_DEMO_WINDOWS': macro redefinition
    	}
		
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Engine/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Engine/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Engine/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/Engine/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Engine/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/Engine/vendor/entt/include"


group "Dependencies"
	include "Engine/vendor/GLFW"
	include "Engine/vendor/Glad"
	include "Engine/vendor/ImGui"

group ""

include "Engine"
include "Editor"