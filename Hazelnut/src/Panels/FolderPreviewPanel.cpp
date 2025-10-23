#include "FolderPreviewPanel.h"
#include "hzpch.h"
#include "FolderPreviewPanel.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <Platform/Vulkan/VulkanTexture.h>
#include "Hazel/Utils/UIUtils.h"
#include <Hazel/Asset/AssetMetadata.h>
#include <Hazel/Asset/AssetImporter.h>

namespace Hazel {

	FolderPreviewPanel::FolderPreviewPanel(const std::filesystem::path& assetsDir)
		: m_AssetsDir(assetsDir), m_CurrentDir(assetsDir)
	{
		// 初始化文件与文件夹图标
		TextureSpecification spec;
		spec.DebugName = "iconForFile";

		std::filesystem::path dirIcon = "Resources/Icons/DirectoryIcon.png";
		std::filesystem::path fileIcon = "Resources/Icons/FileIcon.png";

		m_DirectoryIcon = Texture2D::Create(spec, dirIcon);
		m_FileIcon = Texture2D::Create(spec, fileIcon);
	}
	void FolderPreviewPanel::OnFileOpen(const std::filesystem::path& path)
	{
		std::string ext = path.extension().string();
		if (ext == ".scene")
		{
			HZ_CORE_INFO("Open scene: {}", path.string());
		}
		else if (ext == ".fbx" || ext == ".gltf")
		{
			Ref<MeshSource> meshSource = AssetManager::GetMesh(path);
			Entity meshRoot = m_Context->CreateEntity(path.string());
			if (meshSource->GetAnimationNames().size() == 0) {
				meshRoot.AddComponent<StaticMeshComponent>(meshSource->Handle,path);
			}else {
				m_Context->BuildDynamicMeshEntity(meshSource, meshRoot, path);
			}

		}
		else if (ext == ".png" || ext == ".jpg")
		{
			HZ_CORE_INFO("Preview texture: {}", path.string());

			TextureSpecification spec;
			spec.DebugName = "PreviewImage";
			m_PreviewTexture = Texture2D::Create(spec, path);
			m_PreviewPath = path;
			m_ShowPreview = true;
		}
	}

	void FolderPreviewPanel::SetContext(Ref<Scene>& context)
	{
		m_Context = context;
	}
	void FolderPreviewPanel::OnImGuiRender()
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

			ImGui::ImageButton(UI::GetImageId(icon->GetImage()),
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

		// ================== 预览窗口 ==================
		if (m_ShowPreview)
		{
			bool open = m_ShowPreview;
			ImGui::Begin("Image Preview", &open, ImGuiWindowFlags_AlwaysAutoResize);

			if (m_PreviewTexture)
			{
				ImGui::Text("Path: %s", m_PreviewPath.filename().string().c_str());
				ImGui::Separator();

				float maxWidth = 512.0f;
				float texWidth = (float)m_PreviewTexture->GetWidth();
				float texHeight = (float)m_PreviewTexture->GetHeight();
				float aspect = texHeight / texWidth;

				ImVec2 size = ImVec2(std::min(texWidth, maxWidth), std::min(texWidth, maxWidth) * aspect);
				ImGui::Image(UI::GetImageId(m_PreviewTexture->GetImage()), size);
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load image!");
			}

			ImGui::End();

			if (!open)
			{
				m_ShowPreview = false;
				m_PreviewTexture = nullptr;
				HZ_CORE_INFO("Closed preview window, texture destroyed.");
			}
		}

		// ================== 右键菜单 ==================
		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("New Folder"))
				std::filesystem::create_directory(m_CurrentDir / "New Folder");
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void FolderPreviewPanel::DrawToolbar()
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

} // namespace Hazel
