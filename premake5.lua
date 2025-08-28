workspace "GameEngine"
	architecture "x64"
	startproject "Sandbox" 
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Engine/vendor/GLFW/include"
IncludeDir["Glad"] = "Engine/vendor/Glad/include"

include "Engine/vendor/GLFW"
include "Engine/vendor/Glad"

project "Engine" 
	location "Engine"
	kind "SharedLib"
	language "C++"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "EnginePCH.h"
	pchsource "Engine/src/EnginePCH.cpp"


	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}
	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"ENGINE_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}
		postbuildcommands {
			("{COPY} \"%{cfg.buildtarget.relpath}\" \"%{cfg.buildtarget.directory}/../Sandbox/\"")
		}	

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		defines "ENGINE_ENABLE_ASSERTS"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "ENGINE_DIST"
		buildoptions "/MD"
		optimize "On"

	filter { "system:windows", "configurations:Debug"}
		buildoptions "/utf-8"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	-- 确保先构建Engine
	dependson { "Engine" }

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
		"Engine/src"
	}
	links
	{
		"Engine"
	}
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		defines
		{
			"ENGINE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "ENGINE_DIST"
		buildoptions "/MD"
		optimize "On"

	filter{ "system:windows", "configurations:Debug" }
		buildoptions "/utf-8"