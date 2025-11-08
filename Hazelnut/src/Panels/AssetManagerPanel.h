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
		void ClearState() { m_SelectionContext = {}; m_RenameEntity = {}; }
		void OnImGuiRender() override;

		void SetSelectedEntity(Entity entity);
		Entity GetSelectedEntity() { return m_SelectionContext; }
	private:
		void DrawEntityNode(Entity entity);

		void DrawComponents(Entity entity);

	private:
		static void DrawMaterial(AssetHandle meshSourceHandle);
		
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
		Entity m_RenameEntity;
		char m_RenameBuffer[256]{};
		Ref<Texture2D> m_EntityIcon;
		Ref<Texture2D> m_DirLightIcon;
		Ref<Texture2D> m_SpotLightIcon;
		Ref<Texture2D> m_SkyLightIcon;
		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
	};
}
