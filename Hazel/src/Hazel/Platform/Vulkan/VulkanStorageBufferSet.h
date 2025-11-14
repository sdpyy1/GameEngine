#pragma once

#include "Hazel/Renderer/StorageBufferSet.h"

#include <map>

namespace Hazel {

	class VulkanStorageBufferSet : public StorageBufferSet
	{
	public:
		explicit VulkanStorageBufferSet(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight)
			: m_Specification(specification), m_FramesInFlight(framesInFlight), m_PreSize(size)
		{
			if (framesInFlight == 0)
				m_FramesInFlight = Renderer::GetConfig().FramesInFlight;
			LOG_TRACE("Main: VulkanStorageBufferSet [{0}] Create! size = {1} [Only Submit]", specification.DebugName, m_PreSize);
			for (uint32_t frame = 0; frame < m_FramesInFlight; frame++)
				m_StorageBuffers[frame] = StorageBuffer::Create(size, specification);
		}

		~VulkanStorageBufferSet() override = default;
		uint32_t Get_PreSize() { return m_PreSize; };

		virtual Ref<StorageBuffer> Get() override
		{
			uint32_t frame = Renderer::GetCurrentFrameIndex();
			return Get(frame);
		}

		virtual Ref<StorageBuffer> RT_Get() override
		{
			uint32_t frame = Renderer::RT_GetCurrentFrameIndex();
			return Get(frame);
		}

		virtual Ref<StorageBuffer> Get(uint32_t frame) override
		{
			ASSERT(m_StorageBuffers.find(frame) != m_StorageBuffers.end());
			return m_StorageBuffers.at(frame);
		}

		virtual void Set(Ref<StorageBuffer> storageBuffer, uint32_t frame = 0) override
		{
			m_StorageBuffers[frame] = storageBuffer;
		}

		virtual void Resize(uint32_t newSize) override
		{
			for (uint32_t frame = 0; frame < m_FramesInFlight; frame++)
				m_StorageBuffers.at(frame)->Resize(newSize);
		}
	private:
		StorageBufferSpecification m_Specification;
		uint32_t m_FramesInFlight = 0;
		uint32_t m_PreSize = 0;
		std::map<uint32_t, Ref<StorageBuffer>> m_StorageBuffers;
	};
}
