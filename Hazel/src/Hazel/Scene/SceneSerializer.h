#pragma once

#include "Scene.h"

namespace Hazel {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref_old<Scene>& scene);

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);
	private:
		Ref_old<Scene> m_Scene;
	};

}
