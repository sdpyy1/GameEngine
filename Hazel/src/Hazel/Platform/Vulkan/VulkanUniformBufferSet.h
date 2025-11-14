#pragma once

#include "Hazel/Renderer/UniformBufferSet.h"
#include "Hazel/Renderer/Renderer.h"
#include <map>

namespace Hazel {

	class VulkanUniformBufferSet : public UniformBufferSet
	{
	public:

		VulkanUniformBufferSet(uint32_t size, std::string debugName, uint32_t framesInFlight)
			: m_FramesInFlight(framesInFlight),m_PreSize(size)
		{
			m_DebugName = debugName;
			if (framesInFlight == 0)
				m_FramesInFlight = Renderer::GetConfig().FramesInFlight;
			for (uint32_t frame = 0; frame < m_FramesInFlight; frame++)
				m_UniformBuffers[frame] = UniformBuffer::Create(size,debugName);
		}

		virtual ~VulkanUniformBufferSet() {}

		virtual Ref<UniformBuffer> Get() override
		{
			uint32_t frame = Renderer::GetCurrentFrameIndex();
			return Get(frame);
		}

		virtual Ref<UniformBuffer> RT_Get() override
		{
			uint32_t frame = Renderer::RT_GetCurrentFrameIndex();
			return Get(frame);
		}

		virtual Ref<UniformBuffer> Get(uint32_t frame) override
		{
			ASSERT(m_UniformBuffers.find(frame) != m_UniformBuffers.end());
			return m_UniformBuffers.at(frame);
		}

		virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame = 0) override
		{
			m_UniformBuffers[frame] = uniformBuffer;
		}

		virtual void Set_Data(uint32_t frame, const void* data, uint32_t size, uint32_t offset = 0) override {

			m_UniformBuffers[frame]->SetData(data, size, offset);

		};
		uint32_t Get_PreSize() { return m_PreSize; };
	private:
		uint32_t m_FramesInFlight = 0;
		uint32_t m_PreSize = 0;
		std::map<uint32_t, Ref<UniformBuffer>> m_UniformBuffers;
	};
}
