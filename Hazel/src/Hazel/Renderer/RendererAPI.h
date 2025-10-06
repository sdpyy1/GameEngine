#pragma once

#include "Hazel/Renderer/RendererCapabilities.h"
#include <glm/glm.hpp>
#include "RenderCommandBuffer.h"
#include "RenderPass.h"
#include "Hazel/Renderer/IndexBuffer.h"

namespace Hazel {
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
		virtual void RT_BeginFrame() = 0;
		virtual void RT_EndFrame() = 0;
		virtual void BeginRenderPass(Ref<RenderPass> renderPass) = 0;
		virtual void EndRenderPass() = 0;
		static RendererAPI* CreateAPI();
		virtual void BindVertData(Ref<VertexBuffer> testVertexBuffer) = 0;
		virtual void BindIndexData(Ref<IndexBuffer> indexBuffer) = 0;
		static Type Current() { return s_API; }
	private:
		static Type s_API;
	};

}
