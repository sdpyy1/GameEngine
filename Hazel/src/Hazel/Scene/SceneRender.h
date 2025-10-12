#pragma once
#include "Scene.h"
namespace Hazel {
	class SceneRender
	{
	public:
		SceneRender(Ref<Scene> scene);
		void Init();
		void SubmitStaticMesh(Ref<MeshSource> mesh);
		void CollectRenderableEntities();
	private:
		Ref<Scene> m_scene;


	};

}
