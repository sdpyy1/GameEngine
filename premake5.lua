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
IncludeDir["GLFW"] = "Engine/vendor/GLFW/include"
IncludeDir["Glad"] = "Engine/vendor/Glad/include"
IncludeDir["ImGui"] = "Engine/vendor/ImGui"
IncludeDir["glm"] = "Engine/vendor/glm"
IncludeDir["stb_image"] = "Engine/vendor/stb_image"


group "Dependencies"
	include "Engine/vendor/GLFW"
	include "Engine/vendor/Glad"
	include "Engine/vendor/imgui"

group ""


project "Engine" 
	location "Engine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"


	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "Engine/src/pch.cpp"


	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
		"ENGINE_PLATFORM_WINDOWS"
	}
	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ENGINE_DIST"
		runtime "Release"
		optimize "on"



		project "Editor"
		location "Editor"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
	
		targetdir ("bin/" .. outputdir .. "/%{prj.name}")
		objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
		files
		{
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp"
		}
	
		includedirs
		{
			"Engine/vendor/spdlog/include",
			"Engine/src",
			"Engine/vendor",
			"%{IncludeDir.glm}"
		}
	
		links
		{
			"Engine"
		}
	
		filter "system:windows"
			systemversion "latest"
			defines
			{
				"ENGINE_PLATFORM_WINDOWS"
			}
	
		filter "configurations:Debug"
			defines "HZ_DEBUG"
			runtime "Debug"
			symbols "on"
	
		filter "configurations:Release"
			defines "HZ_RELEASE"
			runtime "Release"
			optimize "on"
	
		filter "configurations:Dist"
			defines "HZ_DIST"
			runtime "Release"
			optimize "on"

-- 被Editor取代
-- project "Sandbox"
-- 	location "Sandbox"
-- 	kind "ConsoleApp"
-- 	language "C++"
-- 	cppdialect "C++17"
-- 	staticruntime "on"

-- 	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
-- 	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

-- 	files
-- 	{
-- 		"%{prj.name}/src/**.h",
-- 		"%{prj.name}/src/**.cpp"
-- 	}
-- 	includedirs
-- 	{
-- 		"Engine/vendor/spdlog/include",
-- 		"Engine/src",
-- 		"%{IncludeDir.glm}",
-- 		"%{IncludeDir.ImGui}",
-- 	}
-- 	links
-- 	{
-- 		"Engine"
-- 	}
-- 	filter "system:windows"
-- 		systemversion "latest"
-- 		defines
-- 		{
-- 			"ENGINE_PLATFORM_WINDOWS"
-- 		}

-- 	filter "configurations:Debug"
-- 		defines "ENGINE_DEBUG"
-- 		runtime "Debug"
-- 		symbols "on"

-- 	filter "configurations:Release"
-- 		defines "ENGINE_RELEASE"
-- 		runtime "Release"
-- 		optimize "on"

-- 	filter "configurations:Dist"
-- 		defines "ENGINE_DIST"
-- 		runtime "Release"
-- 		optimize "on"
