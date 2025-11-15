#pragma once
#include <filesystem>
#include <imgui.h>
#include "Hazel/Core/Base.h"
#include "Hazel/Asset/AssetManager.h"
#include "EditorPanel.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Utils/UIUtils.h"
#include <Hazel/Platform/Vulkan/VulkanTexture.h>
enum class BrowserMode
{
	Directory,
	Category
};
namespace Hazel {
	class FolderPreviewPanel : public EditorPanel
	{
	public:
		FolderPreviewPanel(const std::filesystem::path& assetsDir);

		void OnImGuiRender() override;
		void SetContext(std::shared_ptr<Scene>& context);

	private:
		void DrawToolbar();
		void OnFileOpen(const std::filesystem::path& path);
		void DrawDirectoryTree(const std::filesystem::path& directory);
		void DrawPreviewWindow();
		void DrawFileGrid();
		void ScanAssetsForCategories(const std::filesystem::path& directory);
		void DrawCategoryTree();
	private:
		enum class BrowserMode
		{
			Directory,
			Category
		};

		BrowserMode m_Mode = BrowserMode::Category;
		std::filesystem::path m_AssetsDir;
		std::filesystem::path m_CurrentDir;
		std::shared_ptr<Scene> m_Context;
		std::unordered_set<std::string> m_ExpandedFolders; // ��չ���ļ���·��

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_SceneIcon;
		Ref<Texture2D> m_ModelIcon;
		std::filesystem::path m_SelectedFile;   // ��ǰѡ�е��ļ�
		std::string m_SelectedCategory;         // ��ǰѡ�еķ���
		std::vector<std::filesystem::path> m_ModelFiles;
		std::vector<std::filesystem::path> m_TextureFiles;
		std::vector<std::filesystem::path> m_SceneFiles;
		std::vector<std::filesystem::path> m_CategoryPreviewFiles;

		Ref<Texture2D> m_PreviewTexture;
		bool m_ShowPreview = false;
		std::filesystem::path m_PreviewPath;
	};
}
