#pragma once
#include "Vulkan.h"
#include "VulkanDevice.h"
struct GLFWwindow;
namespace Hazel {
	class VulkanSwapChain
	{
	public:
		VulkanSwapChain() = default;

		VulkanSwapChain(VkInstance m_Instance, const Ref<VulkanDevice>& device);
		void InitSurface(GLFWwindow* windowHandle);
		void Create_old(uint32_t* width, uint32_t* height, bool vsync);
		void BeginFrame();
		void Present();
		std::vector<VkImageView> GetViews() {
			std::vector<VkImageView> res;
			for (unsigned i = 0; i < m_ImageCount; i++) {
				res.push_back(m_Images[i].ImageView);
			}
			return res;
		}
		void Destroy();
		void OnResize(uint32_t width, uint32_t height);
		uint32_t GetImageCount() const { return m_ImageCount; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		VkRenderPass GetRenderPass() { return m_RenderPass; }
		uint32_t GetCurrentBufferIndex() const { return m_CurrentFrameIndex; }
		VkFramebuffer GetCurrentFramebuffer() { return GetFramebuffer(m_CurrentImageIndex); }
		VkCommandBuffer GetCurrentDrawCommandBuffer() { return GetDrawCommandBuffer(m_CurrentFrameIndex); }
		VkFormat GetColorFormat() { return m_ColorFormat; }
		void SetVSync(const bool enabled) { m_VSync = enabled; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkExtent2D GetExtent() { return swapchainExtent; }
		VkCommandBuffer GetDrawCommandBuffer(uint32_t index)
		{
			HZ_CORE_ASSERT(index < m_CommandBuffers.size());
			return m_CommandBuffers[index].CommandBuffer;
		}
		VkFramebuffer GetFramebuffer(uint32_t index)
		{
			HZ_CORE_ASSERT(index < m_Framebuffers.size());
			return m_Framebuffers[index];
		}

	private:
		void FindImageFormatAndColorSpace();
		uint32_t AcquireNextImage();

	private:
		VkInstance m_Instance = nullptr;
		Ref<VulkanDevice> m_Device;
		VkSurfaceKHR m_Surface;
		VkRenderPass m_RenderPass = nullptr;
		std::vector<VkFramebuffer> m_Framebuffers;
		VkExtent2D swapchainExtent;
		uint32_t m_QueueNodeIndex = UINT32_MAX;
		VkFormat m_ColorFormat;
		VkColorSpaceKHR m_ColorSpace;
		bool m_VSync = false;
		VkSwapchainKHR m_SwapChain = nullptr;
		uint32_t m_Width = 0, m_Height = 0;
		struct SwapchainImage
		{
			VkImage Image = nullptr;
			VkImageView ImageView = nullptr;
		};
		std::vector<SwapchainImage> m_Images;
		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_VulkanImages;
		struct SwapchainCommandBuffer
		{
			VkCommandPool CommandPool = nullptr;
			VkCommandBuffer CommandBuffer = nullptr;
		};
		std::vector<SwapchainCommandBuffer> m_CommandBuffers;
		// Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;

		// Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		std::vector<VkFence> m_WaitFences;

		uint32_t m_CurrentFrameIndex = 0;    // Index of the frame we are currently working on, up to max frames in flight
		uint32_t m_CurrentImageIndex = 0;    // Index of the current swapchain image.  Can be different from frame index
	};

}
