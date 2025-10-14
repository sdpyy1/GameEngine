#pragma once
#include <Hazel/Editor/EditorPanel.h>
#include <Hazel.h>
namespace Hazel {
	class AssetManagerPanel : public EditorPanel
	{
	public:
		AssetManagerPanel();
		virtual ~AssetManagerPanel() = default;
		void SetContext(Ref<Scene>& scene);
		virtual void OnImGuiRender() override;
		void DrawEntityNode(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

	};
}
