#pragma once
#include "Volk/volk.h"
#include "Hazel/Renderer/RHI/RHIResource.h"
#include <GLFW/glfw3.h>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace GameEngine {
	class VulkanDynamicRHI;
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
	class VulkanRHISurface : public RHISurface
	{
	public:
		VulkanRHISurface(GLFWwindow* window);

		const VkSurfaceKHR& GetHandle() { return handle; }

		virtual void Destroy() override final;

	private:
		VkSurfaceKHR handle;
	};

	class VulkanRHISwapchain : public RHISwapchain
	{
	public:
		VulkanRHISwapchain(const RHISwapchainInfo& info);

		virtual uint32_t GetCurrentFrameIndex() override final { return currentIndex; }
		virtual RHITextureRef GetTexture(uint32_t index) override final { return textures[index]; }
		virtual RHITextureRef GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore) override final;
		virtual void Present(RHISemaphoreRef waitSemaphore) override final;

		const VkSwapchainKHR& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSwapchainKHR handle;
		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		std::vector<VkImage> images;
		VkFormat imageFormat;
		VkExtent2D imageExtent;

		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> availableFormats;
		std::vector<VkPresentModeKHR> availablePresentModes;

		std::vector<RHITextureRef> textures;
		uint32_t currentIndex;

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkFormat targetFormat);
		VkPresentModeKHR ChooseSwapPresentMode();
		VkExtent2D ChooseSwapExtent();
		void Resize();
	};
	class VulkanRHITexture : public RHITexture
	{
	public:
		VulkanRHITexture(const RHITextureInfo& info, VkImage image = VK_NULL_HANDLE);

		const VkImage& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkImage handle;

		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	class VulkanRHITextureView : public RHITextureView
	{
	public:
		VulkanRHITextureView(const RHITextureViewInfo& info);

		const VkImageView& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkImageView handle;
	};
	class VulkanRHICommandPool : public RHICommandPool
	{
	public:
		VulkanRHICommandPool(const RHICommandPoolInfo& info);

		RHIQueueRef GetQueue() { return info.queue; }

		const VkCommandPool& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkCommandPool handle;
	};

	class VulkanRHISampler : public RHISampler
	{
	public:
		VulkanRHISampler(const RHISamplerInfo& info);

		const VkSampler& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSampler handle;
	};
	class VulkanRHIShader : public RHIShader
	{
	public:
		VulkanRHIShader(const RHIShaderInfo& info);

		VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo();

		const VkShaderModule& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkShaderModule handle;
	};
	class VulkanRHIBuffer : public RHIBuffer
	{
	public:
		VulkanRHIBuffer(const RHIBufferInfo& info);

		const VkBuffer& GetHandle() { return handle; }

		virtual void* Map() override final;
		virtual void UnMap() override final;

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkBuffer handle;

		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		bool mapped = false;
		void* pointer = nullptr;
	};
	//Í¬²½ ////////////////////////////////////////////////////////////////////////////////////////////////////////

	class VulkanRHIFence : public RHIFence
	{
	public:
		VulkanRHIFence(bool signaled);

		virtual void Wait() override final;

		const VkFence& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkFence handle;
	};

	class VulkanRHISemaphore : public RHISemaphore
	{
	public:
		VulkanRHISemaphore();

		const VkSemaphore& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSemaphore handle;
	};

}

