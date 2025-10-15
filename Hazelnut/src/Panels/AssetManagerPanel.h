#pragma once
#include "Hazel.h"
#include "Hazel/Editor/EditorPanel.h"
#include "Hazel/Scene/Scene.h"

namespace Hazel {

	class AssetManagerPanel : public EditorPanel
	{
	public:
		AssetManagerPanel();

		void SetContext(Ref<Scene>& scene);

		virtual void OnImGuiRender() override;

		void SetSelectedEntity(Entity entity);
		Entity GetSelectedEntity() { return m_SelectionContext; }
	private:
		void DrawEntityNode(Entity entity);

		void DrawComponents(Entity entity);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		Entity m_RenameEntity;
		char m_RenameBuffer[256]{};
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
	};

}
