#pragma once

#include <string>
#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
namespace Hazel {
	struct GPUMemoryStats
	{
		uint64_t Used = 0;
		uint64_t TotalAvailable = 0;
		uint64_t AllocationCount = 0;

		uint64_t BufferAllocationSize = 0;
		uint64_t BufferAllocationCount = 0;

		uint64_t ImageAllocationSize = 0;
		uint64_t ImageAllocationCount = 0;
	};
	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);
		~VulkanAllocator();

		//void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer);
		VmaAllocation AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage, VkDeviceSize* allocatedSize = nullptr);
		void Free(VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			vmaMapMemory(VulkanAllocator::GetVMAAllocator(), allocation, (void**)&mappedMemory);
			return mappedMemory;
		}

		void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
		static GPUMemoryStats GetStats();

		static void Init(Ref<VulkanDevice> device);
		static void Init(Ref<VulkanDevice> device, VkInstance instance);
		//void Init(Ref<VulkanDevice> device, VkInstance instance);
		static void Shutdown();

		static VmaAllocator& GetVMAAllocator();
	private:
		std::string m_Tag;
	};
}
