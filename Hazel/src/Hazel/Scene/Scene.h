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
	struct DirLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Intensity = 1.0f;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		float ShadowAmount = 1.0f;
		// C++ only
		bool CastShadows = true;
	};

	struct PointLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float MinRadius = 0.001f;
		float Radius = 25.0f;
		float Falloff = 1.f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct SpotLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		float AngleAttenuation = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Range = 0.1f;
		float Angle = 0.0f;
		float Falloff = 1.0f;
		bool SoftShadows = true;
		char Padding0[3]{ 0, 0, 0 };
		bool CastsShadows = true;
		char Padding1[3]{ 0, 0, 0 };
	};

	struct LightEnvironment
	{
		static constexpr size_t MaxDirectionalLights = 4;

		DirectionalLight DirectionalLights[MaxDirectionalLights];
		std::vector<PointLight> PointLights;
		std::vector<SpotLight> SpotLights;
		[[nodiscard]] uint32_t GetPointLightsSize() const { return (uint32_t)(PointLights.size() * sizeof(PointLight)); }
		[[nodiscard]] uint32_t GetSpotLightsSize() const { return (uint32_t)(SpotLights.size() * sizeof(SpotLight)); }
	};
	struct SceneInfo
	{
		EditorCamera camera;
		LightEnvironment SceneLightEnvironment;
	};
	class Scene : public RefCounted
	{
	public:
		Scene();
		void PackupSceneInfo(EditorCamera& editorCamera);
		~Scene();

		void OnEditorRender(Timestep ts, EditorCamera& editorCamera);
		void UpdateAnimation(Timestep ts);
		void OutputViewport();
		void SetViewprotSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; }
	public:
		void CollectRenderableEntities();
		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateChildEntity(Entity parent, const std::string& name);
		void SortEntities();
		void DestroyEntity(Entity entity);
		Entity DuplicateEntity(Entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);
		void ClearEntities();
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
		entt::registry& GetRegistry() { return m_Registry; }
		Entity BuildDynamicMeshEntity(Ref<MeshSource> mesh, Entity& root, const std::filesystem::path& path);

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
		std::vector<glm::mat4> GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<MeshSource> meshSource);

	private:
		entt::registry m_Registry;
		using EntityMap = std::unordered_map<UUID, Entity>;
		EntityMap m_EntityIDMap;
		float m_ViewportWidth = 1216.0f, m_ViewportHeight = 849.0f;
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneRender;
		SceneInfo m_SceneInfo;
		Ref<SceneRender> m_SceneRender = nullptr;
	};
}
