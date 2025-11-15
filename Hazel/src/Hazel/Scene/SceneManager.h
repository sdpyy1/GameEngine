#pragma once
#include "Hazel/Core/Timestep.h"
#include "Scene.h"
#include "Hazel/Renderer/EditorCamera.h"
namespace Hazel
{
	class SceneManager
	{
	public:
		SceneManager();
		void tick(Timestep ts);

		std::shared_ptr<EditorCamera> GetEditorCamera() { return m_EditorCamera; };
		std::shared_ptr<Scene> GetActiveScene() { return m_CurrentScene; };
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();
		void SaveSceneAs();
	private:
		std::shared_ptr<Scene> m_CurrentScene;
		std::shared_ptr<EditorCamera> m_EditorCamera;


		std::string m_CurrentSceneFilePath;

	};

}
