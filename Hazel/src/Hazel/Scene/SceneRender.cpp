#include "hzpch.h"
#include "SceneRender.h"
#include "Components.h"
#include "Hazel/Asset/AssetManager.h"
#include "Entity.h"
namespace Hazel {
	SceneRender::SceneRender(Ref<Scene> scene) :m_scene(scene)
	{
		Init();
	}
	void SceneRender::Init()
	{
	}
	void SceneRender::SubmitStaticMesh(Ref<MeshSource> mesh) {
	
	
	
	};
	void SceneRender::CollectRenderableEntities()
	{
		auto allEntityOwnMesh = m_scene->GetAllEntitiesWith<StaticMeshComponent>();
		for (auto entity : allEntityOwnMesh) {
			auto& staticMeshComponent = allEntityOwnMesh.get<StaticMeshComponent>(entity);
			if (!staticMeshComponent.Visible) continue;
			Ref<MeshSource> mesh = AssetManager::GetAsset<MeshSource>(staticMeshComponent.StaticMesh);
			Entity e = Entity(entity, m_scene);
			// 这里获取Model变换信息

			// 注册Mesh
			SubmitStaticMesh(mesh);
		}
	}
}
