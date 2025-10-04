#pragma once

#include "Hazel/Renderer/RendererCapabilities.h"
#include <glm/glm.hpp>
#include "RenderCommandBuffer.h"
#include "RenderPass.h"

namespace Hazel {
	// �������ͼ��API�Ľӿ�
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
		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;
				static RendererAPI* CreateAPI();
		static Type Current() { return s_API; }
	private:
		static Type s_API;
	};

}
