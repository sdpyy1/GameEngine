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

	void Scene::OnEditorRender(Timestep ts,Ref<SceneRender> sceneRender,EditorCamera & editorCamera) {
		sceneRender->SetScene(this);
		UpdateAnimation(ts); // ��������
		CollectRenderableEntities(sceneRender);
		sceneRender->PreRender(editorCamera);

		sceneRender->EndRender();
	};

	void Scene::UpdateAnimation(Timestep ts) {
		auto view = GetAllEntitiesWith<AnimationComponent>();
		for (auto e : view) {
			Entity entity = { e, this };
			auto& animComp = entity.GetComponent<AnimationComponent>();

			if (animComp.meshSource == 0 || animComp.BoneEntityIds.empty() || !animComp.CurrentAnimation)
				continue;

			const auto& animation = animComp.CurrentAnimation;
			float duration = animation->GetDuration();

			if (ts > 0.0f) {
				animComp.CurrentTime += ts;
				if (animComp.IsLooping)
					animComp.CurrentTime = std::fmod(animComp.CurrentTime, duration);
				else
					animComp.CurrentTime = std::clamp(animComp.CurrentTime, 0.0f, duration);

				animation->Sample(animComp.CurrentTime, animComp.CurrentPose);

				for (size_t i = 0; i < animComp.BoneEntityIds.size(); ++i) {
					Entity boneEntity = GetEntityByUUID(animComp.BoneEntityIds[i]);
					if (!boneEntity.HasComponent<TransformComponent>())
						continue;
					auto& boneTransform = boneEntity.GetComponent<TransformComponent>();
					const auto& sampled = animComp.CurrentPose.BoneTransforms[i == 0?0:i+1];  // ������ ��Ӧ�ú�����й�ϵ

					boneTransform.Translation = sampled.Translation;
					boneTransform.SetRotation(sampled.Rotation);
					boneTransform.Scale = sampled.Scale;
				}
			}
		}
	}


	void Scene::OutputRenderRes(Ref<SceneRender> sceneRender)
	{
		UI::Image(sceneRender->GetFinalImage(), ImGui::GetContentRegionAvail(), {0, 0}, {1, 1});
	}
	
	void Scene::CollectRenderableEntities(Ref<SceneRender> sceneRender)
	{
		// �ռ�StaticMesh
		auto allEntityOwnMesh = GetAllEntitiesWith<StaticMeshComponent>();
		for (auto entity : allEntityOwnMesh) {
			auto& staticMeshComponent = allEntityOwnMesh.get<StaticMeshComponent>(entity);
			if (!staticMeshComponent.Visible) continue;
			Ref<MeshSource> mesh = AssetManager::GetAsset<MeshSource>(staticMeshComponent.StaticMesh);
			if (mesh == nullptr) continue;
			Entity e = Entity(entity, this);
			glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
			sceneRender->SubmitStaticMesh(mesh, transform);
		}
		// �ռ�SkeletalMesh
		auto allEntityOwnSubmesh = GetAllEntitiesWith<SubmeshComponent>();
		for (auto entity : allEntityOwnSubmesh)
		{
			HZ_PROFILE_SCOPE("Scene-SubmitDynamicMesh");
			auto meshComponent = allEntityOwnSubmesh.get<SubmeshComponent>(entity);
			if (!meshComponent.Visible)
				continue;

			if (auto meshSource = AssetManager::GetAsset<MeshSource>(meshComponent.Mesh); meshSource)
			{
				Entity e = Entity(entity, this);
				glm::mat4 transform = GetWorldSpaceTransformMatrix(e);
				// ������Ͱѹ�����Ϣת��Ϊ��ģ�Ϳռ�ı任
				sceneRender->SubmitMesh(meshSource, meshComponent.SubmeshIndex, transform, GetModelSpaceBoneTransforms(meshComponent.BoneEntityIds, meshSource));
			}
		}
	}
	std::vector<glm::mat4> Scene::GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<MeshSource> meshSource)
	{
		std::vector<glm::mat4> boneTransforms(boneEntityIds.size());
		if (meshSource)
		{
			if (const auto skeleton = meshSource->GetSkeleton(); skeleton)
			{
				// Can get mismatches if user changes which mesh an entity refers to after the bone entities have been set up
				// TODO(0x): need a better way to handle the bone entities
				//HZ_CORE_ASSERT(boneEntityIds.size() == skeleton.GetNumBones(), "Wrong number of boneEntityIds for mesh skeleton!");
				for (uint32_t i = 0; i < std::min(skeleton->GetNumBones(), (uint32_t)boneEntityIds.size()); ++i)
				{
					auto boneEntity = GetEntityByUUID(boneEntityIds[i]);
					glm::mat4 localTransform = boneEntity ? boneEntity.GetComponent<TransformComponent>().GetTransform() : glm::identity<glm::mat4>();
					auto parentIndex = skeleton->GetParentBoneIndex(i);
					boneTransforms[i] = (parentIndex == Skeleton::NullIndex) ? localTransform : boneTransforms[parentIndex] * localTransform;
				}
			}
		}
		return boneTransforms;
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
		if (const auto iter = m_EntityIDMap.find(uuid); iter != m_EntityIDMap.end())
			return iter->second;
		return Entity{};
	}

	Entity Scene::BuildDynamicMeshEntity(Ref<MeshSource> mesh, Entity& root,const std::filesystem::path& path)
	{
		AssetHandle handle = mesh->Handle;
		root.AddComponent<DynamicMeshComponent>(handle,path);
		root.AddComponent<AnimationComponent>(handle, path);
		auto com = root.GetComponent<AnimationComponent>();
		HZ_CORE_INFO("MeshSource Handle: {}", com.meshSource);

		BuildMeshEntityHierarchy(root, mesh,mesh->GetRootNode());
		BuildBoneEntityIds(root);
		return root;
	}

	
	void Scene::BuildBoneEntityIds(Entity entity)
	{
		// �����Entity���а���SubMesh�������Entity����SubMesh����Ĺ�����Ϣ
		BuildMeshBoneEntityIds(entity, entity);

		//// AnimationComponent may not be a direct child of the entity.
		//// We must rebuild the animation component bone entity ids from the oldest ancestor
		// ���������ҵ�˭�����������
		Entity animationEntity = entity;
		Entity parent = entity.GetParent();
		while (parent)
		{
			if (parent.HasComponent<AnimationComponent>())
			{
				animationEntity = parent;
			}
			parent = parent.GetParent();
		}

		BuildAnimationBoneEntityIds(animationEntity, animationEntity);
	}
	void Scene::BuildAnimationBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if(entity.HasComponent<AnimationComponent>()){
			auto &anim = entity.GetComponent<AnimationComponent>();
			anim.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, AssetManager::GetAsset<MeshSource>(anim.meshSource)->GetSkeleton());
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityByUUID(childId);
			BuildAnimationBoneEntityIds(child, rootEntity);
		}
	}
	void Scene::BuildMeshBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if (entity.HasComponent<SubmeshComponent>()) {
			SubmeshComponent& mc = entity.GetComponent<SubmeshComponent>();
			AssetHandle meshSourceHandle = mc.Mesh;
			Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
			mc.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, meshSource->GetSkeleton()); // ��������Ĺ�����Ϣ
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityByUUID(childId);
			BuildMeshBoneEntityIds(child, rootEntity);
		}

	}
	std::vector<UUID> Scene::FindBoneEntityIds(Entity entity, Entity rootEntity, const Skeleton* skeleton)
	{
		std::vector<UUID> boneEntityIds;

		// ��������һ��һ���ң�ֱ���ҵ�Skeleton���еĹ���Entity
		if (skeleton)
		{
			Entity rootParentEntity = rootEntity ? rootEntity.GetParent() : rootEntity;
			{
				auto boneNames = skeleton->GetBoneNames();
				boneEntityIds.reserve(boneNames.size());
				bool foundAtLeastOne = false;
				for (const auto& boneName : boneNames)
				{
					bool found = false;
					Entity e = entity;
					while (e && e != rootParentEntity)
					{
						Entity boneEntity = TryGetDescendantEntityWithTag(e, boneName);
						if (boneEntity)
						{
							boneEntityIds.emplace_back(boneEntity.GetUUID());
							found = true;
							break;
						}
						e = e.GetParent();
					}
					if (found)
						foundAtLeastOne = true;
					else
						boneEntityIds.emplace_back(0);
				}
				if (!foundAtLeastOne)
					boneEntityIds.resize(0);
			}
		}
		return boneEntityIds;
	}
	Entity Scene::TryGetDescendantEntityWithTag(Entity entity, const std::string& tag)
	{
		//HZ_PROFILE_FUNC();
		if (entity)
		{
			if (entity.GetComponent<TagComponent>().Tag == tag)
				return entity;

			for (const auto childId : entity.Children())
			{
				Entity descendant = TryGetDescendantEntityWithTag(GetEntityByUUID(childId), tag);
				if (descendant)
					return descendant;
			}
		}
		return {};
	}
	void Scene::BuildMeshEntityHierarchy(Entity parent, Ref<MeshSource> meshSource,const MeshNode& node)
	{
		const auto& nodes = meshSource->GetNodes();

		// capture meshSource and nodes so we don't have to keep getting them as we recurse
		std::function<void(Entity, const MeshNode&)> recurse = [&](Entity parent, const MeshNode& node)
			{
				// Skip empty root node
				// We should still apply its transform though, as there will sometimes be a 90 degree rotation here
				// particularly for GLTF assets (where DCC tool may have tried to convert Z-up to Y-up)
				if (node.IsRoot() && node.Submeshes.size() == 0)
				{
					for (uint32_t child : node.Children)
					{
						MeshNode childNode = nodes[child];
						childNode.LocalTransform = node.LocalTransform * childNode.LocalTransform;
						recurse(parent, childNode);
					}
					return;
				}

				Entity nodeEntity = CreateChildEntity(parent, node.Name);
				nodeEntity.Transform().SetTransform(node.LocalTransform);
				//nodeEntity.AddComponent<MeshTagComponent>(); // TODO: (0x) Add correct root entity id

				if (node.Submeshes.size() == 1)
				{
					// Node == Mesh in this case
					uint32_t submeshIndex = node.Submeshes[0];
					auto& mc = nodeEntity.AddComponent<SubmeshComponent>(meshSource->Handle, submeshIndex);

					/*if (mesh->ShouldGenerateColliders())
					{
						auto& colliderComponent = nodeEntity.AddComponent<MeshColliderComponent>();
						Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(nodeEntity, colliderComponent);
						colliderComponent.ColliderAsset = colliderAsset->Handle;
						colliderComponent.SubmeshIndex = submeshIndex;
						colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
						nodeEntity.AddComponent<RigidBodyComponent>();
					}*/
				}
				else if (node.Submeshes.size() > 1)
				{
					// Create one entity per child mesh, parented under node
					for (uint32_t i = 0; i < node.Submeshes.size(); i++)
					{
						uint32_t submeshIndex = node.Submeshes[i];

						// NOTE(Yan): original implemenation use to use "mesh name" from assimp;
						//            we don't store that so use node name instead. Maybe we
						//            should store it?
						Entity childEntity = CreateChildEntity(nodeEntity, node.Name);

						//childEntity.AddComponent<MeshTagComponent>(); // TODO: (0x) Add correct root entity id
						childEntity.AddComponent<SubmeshComponent>(meshSource->Handle, submeshIndex);

						/*if (mesh->ShouldGenerateColliders())
						{
							auto& colliderComponent = childEntity.AddComponent<MeshColliderComponent>();
							Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(childEntity, colliderComponent);
							colliderComponent.ColliderAsset = colliderAsset->Handle;
							colliderComponent.SubmeshIndex = submeshIndex;
							colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
							childEntity.AddComponent<RigidBodyComponent>();
						}*/
					}
				}

				for (uint32_t child : node.Children)
					recurse(nodeEntity, nodes[child]);
			};

		recurse(parent, node);
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
	void Scene::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent& component)
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
	void Scene::OnComponentAdded<DynamicMeshComponent>(Entity entity, DynamicMeshComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<SubmeshComponent>(Entity entity, SubmeshComponent& component)
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
