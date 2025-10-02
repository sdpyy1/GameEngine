#pragma once

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const Ref_old<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawLines(const Ref_old<VertexArray>& vertexArray, uint32_t vertexCount) override;
		virtual RendererCapabilities& GetCapabilities() override;
		virtual void Shutdown(){};
		virtual void BeginFrame(){};
		virtual void EndFrame(){};
		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false){};
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer){};

		virtual void SetLineWidth(float width) override;
	};


}
