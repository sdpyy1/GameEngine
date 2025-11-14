include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "AAA_GameEngine"
	architecture "x86_64"
	startproject "Hazel"
	disablewarnings { "4828" }
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "Hazel/vendor/Box2D"
	include "Hazel/vendor/GLFW"
	include "Hazel/vendor/Glad"
	include "Hazel/vendor/msdf-atlas-gen"
	include "Hazel/vendor/imgui"
	include "Hazel/vendor/yaml-cpp"
	include "Hazel/vendor/NFD-Extended"
group ""

include "Hazel"
