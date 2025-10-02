#pragma once
#include "Hazel/Renderer/VertexBuffer.h"

#include "Hazel/Core/Buffer1.h"

#include "VulkanAllocator.h"

namespace Hazel {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(void* data, uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		VulkanVertexBuffer(uint64_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);

		virtual ~VulkanVertexBuffer() override;

		virtual void SetData(void* buffer, uint64_t size, uint64_t offset = 0) override;
		virtual void RT_SetData(void* buffer, uint64_t size, uint64_t offset = 0) override;
		virtual void Bind() const override {}

		virtual unsigned int GetSize() const override { return m_Size; }
		virtual RendererID GetRendererID() const override { return 0; }

		VkBuffer GetVulkanBuffer() const { return m_VulkanBuffer; }
	private:
		uint64_t m_Size = 0;
		Buffer1 m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
	};

}
