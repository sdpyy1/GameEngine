#pragma once

#include "Hazel/Renderer/VertexArray.h"

namespace Hazel {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const Ref_old<VertexBuffer_old>& vertexBuffer) override;
		virtual void SetIndexBuffer(const Ref_old<IndexBuffer_old>& indexBuffer) override;

		virtual const std::vector<Ref_old<VertexBuffer_old>>& GetVertexBuffers() const { return m_VertexBuffers; }
		virtual const Ref_old<IndexBuffer_old>& GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		uint32_t m_RendererID;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<Ref_old<VertexBuffer_old>> m_VertexBuffers;
		Ref_old<IndexBuffer_old> m_IndexBuffer;
	};

}
