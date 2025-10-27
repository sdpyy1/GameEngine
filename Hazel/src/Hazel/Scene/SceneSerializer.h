#pragma once

#include "Scene.h"

namespace Hazel {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		inline static std::string_view DefaultExtension = ".hscene";

	private:
		Ref<Scene> m_Scene;
	};

}
