#pragma once
#include "vulkan/vulkan.h"
#include "Hazel/Renderer/RHI/RHIResource.h"
namespace Hazel {
	class VulkanRHIQueue : public RHIQueue
	{
	public:
		VulkanRHIQueue(const RHIQueueInfo& info, VkQueue queue, uint32_t queueFamilyIndex): RHIQueue(info), handle(queue), queueFamilyIndex(queueFamilyIndex){}
		virtual void WaitIdle() override final { vkQueueWaitIdle(handle); }
		virtual void* RawHandle() override final { return handle; };
		const VkQueue& GetHandle() { return handle; }
		uint32_t GetQueueFamilyIndex() { return queueFamilyIndex; }
	private:
		VkQueue handle;
		uint32_t queueFamilyIndex;
	};
}

