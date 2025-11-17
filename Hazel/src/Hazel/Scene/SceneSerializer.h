#pragma once

#include "Scene.h"

namespace GameEngine {

	class SceneSerializer
	{
	public:
		SceneSerializer(const std::shared_ptr<Scene> scene);

		void Serialize(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		inline static std::string_view DefaultExtension = ".hscene";

	private:
		std::shared_ptr<Scene> m_Scene;
	};

}
