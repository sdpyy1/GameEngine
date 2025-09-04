#pragma once

#include "entt.hpp"


#include "Engine/Core/Timestep.h"

namespace Engine {
	class Entity;
	class Scene
	{
	public:
		Scene();
		~Scene();
		void OnViewportResize(uint32_t width, uint32_t height);


		Entity CreateEntity(const std::string& name = std::string());

		void OnUpdate(Timestep ts);
	private:
		entt::registry m_Registry;
		friend class Entity;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;


	};

}