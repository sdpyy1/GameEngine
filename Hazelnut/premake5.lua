project "Hazelnut"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Hazel/vendor/spdlog/include",
		"%{wks.location}/Hazel/src",
		"%{wks.location}/Hazel/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.VulkanSDK}/Include",
		"%{IncludeDir.VMA}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",
	}
	-- includedirs
	-- {
	-- 	"src",
	-- 	"vendor/spdlog/include",
	-- 	"%{IncludeDir.Box2D}",
	-- 	"%{IncludeDir.filewatch}",
	-- 	"%{IncludeDir.GLFW}",
	-- 	"%{IncludeDir.Glad}",
	-- 	"%{IncludeDir.ImGui}",
	-- 	"%{IncludeDir.glm}",
	-- 	"%{IncludeDir.msdfgen}",
	-- 	"%{IncludeDir.msdf_atlas_gen}",
	-- 	"%{IncludeDir.stb_image}",
	-- 	"%{IncludeDir.entt}",
	-- 	"%{IncludeDir.mono}",
	-- 	"%{IncludeDir.yaml_cpp}",
	-- 	"%{IncludeDir.ImGuizmo}",
	-- 	"%{IncludeDir.VMA}",
	-- 	"%{IncludeDir.VulkanSDK}/Include",
	-- 	"%{IncludeDir.choc}",
	-- 	"%{IncludeDir.nfd}"
	-- }
	-- libdirs
	-- {
	-- 	"%{LibraryDir.assimp}/assimp-vc143-mtd.lib"
	-- }
	links
	{
		"Hazel"
	}

    filter "configurations:Debug"
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
		}
	filter "system:windows"
		systemversion "latest"

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
