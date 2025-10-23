#pragma once
#include <filesystem>
#include <imgui/imgui.h>
#include "Hazel/Core/Base.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/EditorPanel.h"
#include "Hazel/Utils/UIUtils.h"
#include <Platform/Vulkan/VulkanTexture.h>
#include <Hazel.h>

namespace Hazel {

	class FolderPreviewPanel : public EditorPanel
	{
	public:
		FolderPreviewPanel(const std::filesystem::path& assetsDir);

		void OnImGuiRender() override;
		void SetContext(Ref<Scene>& scene);

	private:
		void DrawToolbar();
		void OnFileOpen(const std::filesystem::path& path);

	private:
		std::filesystem::path m_AssetsDir;
		std::filesystem::path m_CurrentDir;
		Ref<Scene> m_Context;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		std::map<std::filesystem::path, Ref<Asset>> MesheSourceCacheMap;
		Ref<Texture2D> m_PreviewTexture;
		bool m_ShowPreview = false;
		std::filesystem::path m_PreviewPath;
	};

}
