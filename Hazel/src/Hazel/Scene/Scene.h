#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/EditorCamera.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "entt.hpp"
#include <Hazel/Renderer/RenderContext.h>
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Renderer/IndexBuffer.h>
#include <Hazel/Asset/Model/Mesh.h>
class b2World;

namespace Hazel {
	class SceneRender;
	class Entity;

	class Scene: public RefCounted
	{
	public:
		Scene();
		~Scene();
		
		void OnEditorRender(Timestep ts,Ref<SceneRender> sceneRender,EditorCamera& editorCamera);
		void UpdateAnimation(Timestep ts);
		void OutputRenderRes(Ref<SceneRender> sceneRender);
		void SetViewprotSize(float width, float height) {  ViewportWidth = width; ViewportHeight = height;}
	public:
		void CollectRenderableEntities(Ref<SceneRender> sceneRender);
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
		Entity BuildDynamicMeshEntity(Ref<MeshSource> mesh,Entity root);

	private:
		void BuildMeshBoneEntityIds(Entity entity, Entity rootEntity);
		Entity TryGetDescendantEntityWithTag(Entity entity, const std::string& tag);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		void BuildBoneEntityIds(Entity entity);
		void BuildAnimationBoneEntityIds(Entity entity, Entity rootEntity);
		void BuildMeshEntityHierarchy(Entity parent, Ref<MeshSource> mesh, const MeshNode& node);
		std::vector<UUID> FindBoneEntityIds(Entity entity, Entity rootEntity, const Skeleton* skeleton);
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
		std::vector<glm::mat4> GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<MeshSource> meshSource);
	};

}
