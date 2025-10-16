#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/EditorCamera.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "entt.hpp"
#include <Hazel/Renderer/RenderContext.h>
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Renderer/IndexBuffer.h>
class b2World;

namespace Hazel {
	class SceneRender;
	class Entity;

	class Scene: public RefCounted
	{
	public:
		Scene();
		~Scene();
		
		void OnEditorRender(Ref<SceneRender> sceneRender,EditorCamera& editorCamera);
		void OutputRenderRes(Ref<SceneRender> sceneRender);
		void SetViewprotSize(float width, float height) {  ViewportWidth = width; ViewportHeight = height;}
	public:
		void CollectRenderableEntities(Ref<SceneRender> sceneRender);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateChildEntity(Entity parent, const std::string& name);
		void SortEntities();
		void DestroyEntity(Entity entity);
		Entity DuplicateEntity(Entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
		entt::registry& GetRegistry() { return m_Registry; }
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		entt::registry m_Registry;

		b2World* m_PhysicsWorld = nullptr;

		using EntityMap = std::unordered_map<UUID, Entity>;

		EntityMap m_EntityIDMap;
		float ViewportWidth = 1216.0f, ViewportHeight = 849.0f;
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneRender;
	};

}
