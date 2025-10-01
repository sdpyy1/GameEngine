#pragma once

#include "Hazel/Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace Hazel {

	class RendererAPI
	{
	public:
		enum class APIType
		{
			None = 0, OpenGL = 1, Vulkan=2
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const Ref_old<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawLines(const Ref_old<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
		
		virtual void SetLineWidth(float width) = 0;

		static APIType GetAPI() { return s_API; }
		static Scope<RendererAPI> Create_old();
	private:
		static APIType s_API;
	};

}
