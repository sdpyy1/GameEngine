#pragma once

#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/RendererCapabilities.h"
#include <glm/glm.hpp>
#include "RenderCommandBuffer.h"
#include "RenderPass.h"

namespace Hazel {

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
		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const Ref_old<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawLines(const Ref_old<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
		
		virtual void SetLineWidth(float width) = 0;

		static Type Current() { return s_API; }
		static Scope<RendererAPI> Create_old();
	private:
		static Type s_API;
	};

}
