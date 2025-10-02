#pragma once

#include "Hazel/Renderer/Buffer.h"

#include <memory>

namespace Hazel {

	class VertexArray
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(const Ref_old<VertexBuffer_old>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const Ref_old<IndexBuffer_old>& indexBuffer) = 0;

		virtual const std::vector<Ref_old<VertexBuffer_old>>& GetVertexBuffers() const = 0;
		virtual const Ref_old<IndexBuffer_old>& GetIndexBuffer() const = 0;

		static Ref_old<VertexArray> Create_old();
	};

}
