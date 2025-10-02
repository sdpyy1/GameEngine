#include "hzpch.h"
#include "Hazel/Renderer/Buffer.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Hazel {

	Ref_old<VertexBuffer_old> VertexBuffer_old::Create_old(uint32_t size)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::Type::OpenGL:  return CreateRef<OpenGLVertexBuffer>(size);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref_old<VertexBuffer_old> VertexBuffer_old::Create_old(float* vertices, uint32_t size)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::Type::OpenGL:  return CreateRef<OpenGLVertexBuffer>(vertices, size);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref_old<IndexBuffer_old> IndexBuffer_old::Create_old(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::Type::OpenGL:  return CreateRef<OpenGLIndexBuffer>(indices, size);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
