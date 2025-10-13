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

	public:
		void CollectRenderableEntities();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);
		Entity DuplicateEntity(Entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);
		void SubmitStaticMesh(Ref<MeshSource> mesh);
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		entt::registry m_Registry;

		b2World* m_PhysicsWorld = nullptr;

		std::unordered_map<UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneRender;
	};

}
