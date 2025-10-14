#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Hazel/Core/Application.h"
#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Physics/Physics2D.h"
#include "Hazel/Renderer/Renderer.h"
#define GLM_FORCE_DEPTH_ZERO_TO_FE 
#include <glm/glm.hpp>

#include "Entity.h"
#include "Hazel/Utils/UIUtils.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include <Platform/Vulkan/VulkanShader.h>
#include <Platform/Vulkan/VulkanVertexBuffer.h>
#include <Platform/Vulkan/VulkanIndexBuffer.h>
#include <Platform/Vulkan/VulkanUniformBuffer.h>
#include <Platform/Vulkan/VulkanTexture.h>
#include <Platform/Vulkan/VulkanFramebuffer.h>
#include <Platform/Vulkan/VulkanPipeline.h>
#include <imgui.h>
#include <Hazel/Asset/AssetImporter.h>
#include <Hazel/Asset/Model/Mesh.h>
#include <Platform/Vulkan/VulkanMaterial.h>
#include "SceneRender.h"

namespace Hazel {
	Scene::Scene()
	{
	}



	void Scene::OnEditorRender(Ref<SceneRender> sceneRender,EditorCamera & editorCamera) {
		sceneRender->SetScene(this);

		CollectRenderableEntities(sceneRender);
		sceneRender->PreRender(editorCamera);


		sceneRender->EndRender();
	};



	void Scene::OutputRenderRes(Ref<SceneRender> sceneRender)
	{
		UI::Image(sceneRender->GetFinalImage(), ImGui::GetContentRegionAvail(), {0, 0}, {1, 1});
	}
	

	void Scene::CollectRenderableEntities(Ref<SceneRender> sceneRender)
	{
		auto allEntityOwnMesh = GetAllEntitiesWith<StaticMeshComponent>();
		for (auto entity : allEntityOwnMesh) {
			auto& staticMeshComponent = allEntityOwnMesh.get<StaticMeshComponent>(entity);
			if (!staticMeshComponent.Visible) continue;
			Ref<MeshSource> mesh = AssetManager::GetAsset<MeshSource>(staticMeshComponent.StaticMesh);
			if (mesh == nullptr) continue;
			Entity e = Entity(entity, this);
			// 这里获取Model变换信息
			glm::mat4 transform = GetWorldSpaceTransformMatrix(e);

			// 注册Mesh
			sceneRender->SubmitStaticMesh(mesh, transform);
		}
	}
	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = GetEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
	}
	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

				auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}


	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateChildEntity({}, name);

	}
	
	Entity Scene::CreateChildEntity(Entity parent, const std::string& name)
	{
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = {};

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		if (parent)
			entity.SetParent(parent);

		m_EntityIDMap[idComponent.ID] = entity;

		SortEntities();

		return entity;
	}
	void Scene::SortEntities()
	{
		m_Registry.sort<IDComponent>([&](const auto lhs, const auto rhs)
			{
				auto lhsEntity = m_EntityIDMap.find(lhs.ID);
				auto rhsEntity = m_EntityIDMap.find(rhs.ID);
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});
	}
	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityIDMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		// TODO(Yan): Maybe should be assert
		if (const auto iter = m_EntityIDMap.find(uuid); iter != m_EntityIDMap.end())
			return iter->second;
		return Entity{};
	}


	Scene::~Scene()
	{
		delete m_PhysicsWorld;
	}

  template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(sizeof(T) == 0);
	}
	template<>
	void Scene::OnComponentAdded<StaticMeshComponent>(Entity entity, StaticMeshComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	//template<>
	//void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	//{
	//	if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
	//		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	//}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

}
