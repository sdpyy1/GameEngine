#pragma once
#include "Scene.h"
namespace Hazel {
	class SceneRender : public RefCounted
	{
	public:
		SceneRender();
		Ref<Image2D> GetFinalImage() { return m_GeoFrameBuffer->GetImage(2); }
		void SetScene(Scene* scene) { m_scene = scene; }
		void test(); // TODO:µ÷ÊÔÓÃ
		void PreRender(EditorCamera& camera);
		void EndRender();
	private:
		// Pass
		void GeoPass();
	private:
		void Init();
		void Draw();
		void UpdateMVPMatrix(EditorCamera& camera);
	private:
		// TODO:µ÷ÊÔ
		Ref<VertexBuffer> testVertexBuffer;
		Ref<IndexBuffer> indexBuffer;


		Scene* m_scene;
		// command buffer
		Ref<RenderCommandBuffer> m_CommandBuffer;

		// uniform buffer
		struct UniformBufferObject {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};
		UniformBufferObject* m_MVPMatrix = nullptr;
		Ref<UniformBufferSet> m_MVPUniformBufferSet;


		// GeoPass
		Ref<RenderPass> m_GeoPass;
		Ref<Pipeline> m_GeoPipeline;
		Ref<Framebuffer> m_GeoFrameBuffer;


	};

}
