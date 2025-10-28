#include "hzpch.h"
#include "FolderPreviewPanel.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include "Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Utils/UIUtils.h"
#include "Hazel/Asset/AssetMetadata.h"
#include "Hazel/Asset/AssetImporter.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"

namespace Hazel {

	FolderPreviewPanel::FolderPreviewPanel(const std::filesystem::path& assetsDir)
		: m_AssetsDir(assetsDir), m_CurrentDir(assetsDir)
	{
		TextureSpecification spec;
		spec.DebugName = "FolderIcons";

		std::filesystem::path dirIcon = "Resources/Icons/DirectoryIcon.png";
		std::filesystem::path fileIcon = "Resources/Icons/FileIcon.png";

		m_DirectoryIcon = Texture2D::Create(spec, dirIcon);
		m_FileIcon = Texture2D::Create(spec, fileIcon);
	}

	void FolderPreviewPanel::SetContext(Ref<Scene>& context)
	{
		m_Context = context;
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
			if (meshSource->GetAnimationNames().empty())
				meshRoot.AddComponent<StaticMeshComponent>(meshSource->Handle, path);
			else
			{
				meshRoot.AddComponent<DynamicMeshComponent>(meshSource->Handle, path);
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

	void FolderPreviewPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		DrawToolbar();
		ImGui::Separator();

		const float padding = 12.0f;
		const float thumbnailSize = 90.0f;
		const float cellSize = thumbnailSize + padding;

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

	for (auto& entry : std::filesystem::directory_iterator(m_CurrentDir))
	{
		const auto& path = entry.path();
		std::string name = path.filename().string();

		ImGui::PushID(name.c_str());
		ImGui::BeginGroup();

		Ref<Texture2D> icon = entry.is_directory() ? m_DirectoryIcon : m_FileIcon;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f,0.3f,0.3f,0.35f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f,0.5f,0.7f,0.45f));

		// 1. 为按钮生成唯一 ID（确保每个按钮的 ID 唯一，避免冲突）
		ImGuiID imageBtnId = ImGui::GetID(("custom_image_btn_" + std::to_string(123123)).c_str());

		// 2. 使用 ImageButtonEx 绘制按钮，显式传入 ID 和其他参数
		bool isClicked = ImGui::ImageButtonEx(imageBtnId, UI::GetImageId(icon->GetImage()), { 90.0f, 90.0f }, { 0, 1 }, { 1, 0 }, ImVec2(0, 0), ImVec4(0, 0, 0, 0), ImVec4(255, 255, 255, 255)
		);
		ImGui::PopStyleColor(3);

		ImGui::SetItemAllowOverlap();

		// 拖拽逻辑
		if (ImGui::BeginDragDropSource())
		{
			std::string absPath = std::filesystem::absolute(path).string();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", absPath.c_str(), absPath.size() + 1);
			ImGui::Text("Dragging: %s", absPath.c_str());
			ImGui::EndDragDropSource();
		}

		// 双击打开逻辑
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (entry.is_directory())
				m_CurrentDir /= path.filename();
			else
				OnFileOpen(path);
		}

		// 悬停提示
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", path.string().c_str());

		ImGui::TextWrapped(name.c_str());
		ImGui::EndGroup();
		ImGui::NextColumn();
		ImGui::PopID();
	}
		ImGui::Columns(1);

		// ================== 图片预览窗口 ==================
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
				Renderer::SubmitResourceFree([texture = m_PreviewTexture]() mutable {
					texture = nullptr;
					});
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
