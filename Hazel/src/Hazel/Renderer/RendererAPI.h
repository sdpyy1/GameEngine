#pragma once

#include "Hazel/Renderer/RendererCapabilities.h"
#include <glm/glm.hpp>
#include "RenderCommandBuffer.h"
#include "RenderPass.h"
#include "Hazel/Renderer/IndexBuffer.h"

namespace Hazel {
	class Material;
	class MeshSource;
	// 抽象各种图像API的接口
	class RendererAPI
	{
	public:
		enum class Type
		{
			None = 0, OpenGL = 1, Vulkan=2
		};
		enum class PrimitiveType
		{
			None = 0, Triangles, Lines
		};
	public:
		virtual ~RendererAPI() = default;
		virtual RendererCapabilities& GetCapabilities() = 0;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool explicitClear) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;
		virtual void RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount) = 0;
		virtual void RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount) = 0;
		static RendererAPI* CreateAPI();
		virtual void BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer) = 0;
		virtual void DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count) = 0;
		static Type Current() { return s_API; }
	private:
		static Type s_API;
	};

}
