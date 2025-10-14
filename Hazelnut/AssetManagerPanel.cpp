#include "AssetManagerPanel.h"
#include <imgui.h>
namespace Hazel {
	AssetManagerPanel::AssetManagerPanel()
	{
	}
	void AssetManagerPanel::SetContext(Ref<Scene>& scene)
	{
		m_Context = scene;
	}
	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin("AssetManager");

		auto view = m_Context->GetAllEntitiesWith<IDComponent>();
		for (auto entity : view)
		{
			Entity e(entity, m_Context);
			DrawEntityNode(e);
		}

		ImGui::End();
	}

	void AssetManagerPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetName();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());

		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		if (opened)
		{
			// 递归绘制子节点
			for (UUID childID : entity.Children())
			{
				Entity child = m_Context->GetEntityByUUID(childID);
				DrawEntityNode(child);
			}
			ImGui::TreePop();
		}
	}
}

