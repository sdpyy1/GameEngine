#pragma once
#include <filesystem>
#include <imgui/imgui.h>
#include "Hazel/Core/Base.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/EditorPanel.h"
#include "Hazel/Utils/UIUtils.h"

namespace Hazel {

	class FolderPreviewPanel : public EditorPanel
	{
	public:
		FolderPreviewPanel(const std::filesystem::path& assetsDir)
			: m_AssetsDir(assetsDir), m_CurrentDir(assetsDir)
		{
			// 图标资源（可用占位图）
			TextureSpecification spec;
			spec.DebugName = "iconForFile";
			std::filesystem::path dirIcon= "Resources/Icons/DirectoryIcon.png";
			std::filesystem::path fileIcon = "Resources/Icons/FileIcon.png";
			m_DirectoryIcon = Texture2D::Create(spec, dirIcon);
			m_FileIcon = Texture2D::Create(spec, fileIcon);
		}

		void OnImGuiRender()
		{
			ImGui::Begin("Content Browser");

			DrawToolbar();
			ImGui::Separator();

			float padding = 12.0f;
			float thumbnailSize = 90.0f;
			float cellSize = thumbnailSize + padding;

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = (int)(panelWidth / cellSize);
			if (columnCount < 1)
				columnCount = 1;

			ImGui::Columns(columnCount, 0, false);

			for (auto& entry : std::filesystem::directory_iterator(m_CurrentDir))
			{
				const auto& path = entry.path();
				std::string name = path.filename().string();

				ImGui::PushID(name.c_str());

				Ref<Texture2D> icon = entry.is_directory() ? m_DirectoryIcon : m_FileIcon;

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.7f, 0.45f));
				ImGui::ImageButton((ImTextureID)UI::GetImageId(icon->GetImage()),
					{ thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

				ImGui::PopStyleColor(3);

				// 双击打开文件夹 / 预览文件
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.is_directory())
						m_CurrentDir /= path.filename();
					else
						OnFileOpen(path);
				}

				// 悬停提示完整路径
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(path.string().c_str());

				// 文件名文本
				ImGui::TextWrapped(name.c_str());

				ImGui::NextColumn();
				ImGui::PopID();
			}

			ImGui::Columns(1);

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem("New Folder"))
					std::filesystem::create_directory(m_CurrentDir / "New Folder");

				if (ImGui::MenuItem("Refresh"))
					; // 刷新逻辑（懒加载缓存可以后续做）

				ImGui::EndPopup();
			}

			ImGui::End();
		}

	private:
		void DrawToolbar()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

			if (m_CurrentDir != m_AssetsDir)
			{
				if (ImGui::Button("<-"))
					m_CurrentDir = m_CurrentDir.parent_path();
				ImGui::SameLine();
			}

			ImGui::TextDisabled("Path:");
			ImGui::SameLine();

			std::string displayPath = std::filesystem::absolute(m_CurrentDir).string();
			if (displayPath.empty()) displayPath = "Assets";
			ImGui::Text(displayPath.c_str());

			ImGui::PopStyleVar();
		}

		void OnFileOpen(const std::filesystem::path& path)
		{
			std::string ext = path.extension().string();
			if (ext == ".scene")
			{
				HZ_CORE_INFO("Open scene: {}", path.string());
			}
			else if (ext == ".fbx" || ext == ".gltf")
			{
				HZ_CORE_INFO("Import model: {}", path.string());
			}
			else if (ext == ".png" || ext == ".jpg")
			{
				HZ_CORE_INFO("Preview texture: {}", path.string());
			}
		}

	private:
		std::filesystem::path m_AssetsDir;
		std::filesystem::path m_CurrentDir;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
	};

}
