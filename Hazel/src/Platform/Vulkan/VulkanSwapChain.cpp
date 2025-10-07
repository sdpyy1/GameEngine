#include "hzpch.h"
#include "VulkanSwapChain.h"
#include "VulkanUtils.h"
#include <GLFW/glfw3.h>
#include <spdlog/fmt/ostr.h>
#include "Hazel/Renderer/Renderer.h"
#include "VulkanShader.h"
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
static PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
static PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
static PFN_vkQueuePresentKHR fpQueuePresentKHR;
// Nvidia extensions
PFN_vkCmdSetCheckpointNV fpCmdSetCheckpointNV;
PFN_vkGetQueueCheckpointDataNV fpGetQueueCheckpointDataNV;
namespace Hazel
{
	VulkanSwapChain::VulkanSwapChain(VkInstance instance, const Ref<VulkanDevice>& device)
	{
		m_Instance = instance;
		m_Device = device;
		VkDevice vulkanDevice = m_Device->GetVulkanDevice();
		GET_DEVICE_PROC_ADDR(vulkanDevice, CreateSwapchainKHR);
		GET_DEVICE_PROC_ADDR(vulkanDevice, DestroySwapchainKHR);
		GET_DEVICE_PROC_ADDR(vulkanDevice, GetSwapchainImagesKHR);
		GET_DEVICE_PROC_ADDR(vulkanDevice, AcquireNextImageKHR);
		GET_DEVICE_PROC_ADDR(vulkanDevice, QueuePresentKHR);

		GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_PROC_ADDR(m_Instance, GetPhysicalDeviceSurfacePresentModesKHR);

		GET_INSTANCE_PROC_ADDR(m_Instance, CmdSetCheckpointNV);
		GET_INSTANCE_PROC_ADDR(m_Instance, GetQueueCheckpointDataNV);
	}
	void VulkanSwapChain::BeginFrame() {
		// Resource release queue
		auto& queue = Renderer::GetRenderResourceReleaseQueue(m_CurrentFrameIndex);
		queue.Execute();
		// ��ȡ��һ֡ͼƬ
		m_CurrentImageIndex = AcquireNextImage();
		// �ڲ���CommandBufferҲ��Reset
		VK_CHECK_RESULT(vkResetCommandPool(m_Device->GetVulkanDevice(), m_CommandBuffers[m_CurrentFrameIndex].CommandPool, 0));

	}

	// ��Ⱦ���������APP����
	void VulkanSwapChain::Present()
	{
		VkCommandBuffer commandBuffer = GetCurrentDrawCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitStageMask;
		submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrameIndex].CommandBuffer;
		submitInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkResetFences(m_Device->GetVulkanDevice(), 1, &m_WaitFences[m_CurrentFrameIndex]));

		m_Device->LockQueue();
		VK_CHECK_RESULT(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_WaitFences[m_CurrentFrameIndex]));

		// Present the current buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		VkResult result;
		{

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = NULL;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_SwapChain;
			presentInfo.pImageIndices = &m_CurrentImageIndex;

			presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
			presentInfo.waitSemaphoreCount = 1;
			result = fpQueuePresentKHR(m_Device->GetGraphicsQueue(), &presentInfo);
		}

		m_Device->UnlockQueue();

		if (result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				OnResize(m_Width, m_Height);
			}
			else
			{
				VK_CHECK_RESULT(result);
			}
		}
	}

	void VulkanSwapChain::InitSurface(GLFWwindow* windowHandle)
	{
		VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();

		glfwCreateWindowSurface(m_Instance, windowHandle, nullptr, &m_Surface);

		// Get available queue family properties
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		HZ_CORE_ASSERT(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

		// Iterate over each queue to learn whether it supports presenting:
		// Find a queue with present support
		// Will be used to present the swap chain images to the windowing system
		std::vector<VkBool32> supportsPresent(queueCount);
		for (uint32_t i = 0; i < queueCount; i++)
		{
			fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &supportsPresent[i]);
		}

		// Search for a graphics and a present queue in the array of queue
		// families, try to find one that supports both
		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueCount; i++)
		{
			if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				if (graphicsQueueNodeIndex == UINT32_MAX)
				{
					graphicsQueueNodeIndex = i;
				}

				if (supportsPresent[i] == VK_TRUE)
				{
					graphicsQueueNodeIndex = i;
					presentQueueNodeIndex = i;
					break;
				}
			}
		}
		if (presentQueueNodeIndex == UINT32_MAX)
		{
			// If there's no queue that supports both present and graphics
			// try to find a separate present queue
			for (uint32_t i = 0; i < queueCount; ++i)
			{
				if (supportsPresent[i] == VK_TRUE)
				{
					presentQueueNodeIndex = i;
					break;
				}
			}
		}

		HZ_CORE_ASSERT(graphicsQueueNodeIndex != UINT32_MAX);
		HZ_CORE_ASSERT(presentQueueNodeIndex != UINT32_MAX);

		m_QueueNodeIndex = graphicsQueueNodeIndex;

		FindImageFormatAndColorSpace();
	}

	void VulkanSwapChain::Create(uint32_t* width, uint32_t* height, bool vsync)
	{
		m_VSync = vsync;

		VkDevice device = m_Device->GetVulkanDevice();
		VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();

		VkSwapchainKHR oldSwapchain = m_SwapChain;

		// Get physical device surface properties and formats
		VkSurfaceCapabilitiesKHR surfCaps;
		VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfCaps));
		// Get available present modes
		uint32_t presentModeCount;
		VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, NULL));
		HZ_CORE_ASSERT(presentModeCount > 0);
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data()));

		swapchainExtent = {};
		// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
		if (surfCaps.currentExtent.width == (uint32_t)-1)
		{
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent.width = *width;
			swapchainExtent.height = *height;
		}
		else
		{
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfCaps.currentExtent;
			*width = surfCaps.currentExtent.width;
			*height = surfCaps.currentExtent.height;
		}

		m_Width = *width;
		m_Height = *height;
		if (*width == 0 || *height == 0)
			return;
		// Select a present mode for the swapchain

			// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
			// This mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		// If v-sync is not requested, try to find a mailbox mode
		// It's the lowest latency non-tearing present mode available
		if (!vsync)
		{
			for (size_t i = 0; i < presentModeCount; i++)
			{
				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}
		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
		{
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}
		// Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			// We prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			preTransform = surfCaps.currentTransform;
		}

		// Find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// Simply select the first composite alpha format available
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for (auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}
		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = NULL;
		swapchainCI.surface = m_Surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = m_ColorFormat;
		swapchainCI.imageColorSpace = m_ColorSpace;
		swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = oldSwapchain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		// Enable transfer source on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		// Enable transfer destination on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VK_CHECK_RESULT(fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &m_SwapChain));
		if (oldSwapchain)
			fpDestroySwapchainKHR(device, oldSwapchain, nullptr);
		for (auto& image : m_Images)
			vkDestroyImageView(device, image.ImageView, nullptr);
		m_Images.clear();

		VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, m_SwapChain, &m_ImageCount, NULL));
		// Get the swap chain images
		m_Images.resize(m_ImageCount);
		m_VulkanImages.resize(m_ImageCount);
		VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, m_SwapChain, &m_ImageCount, m_VulkanImages.data()));
		// Get the swap chain buffers containing the image and imageview
		m_Images.resize(m_ImageCount);
		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = NULL;
			colorAttachmentView.format = m_ColorFormat;
			colorAttachmentView.image = m_VulkanImages[i];
			colorAttachmentView.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;

			m_Images[i].Image = m_VulkanImages[i];

			VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &m_Images[i].ImageView));
			VKUtils::SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("Swapchain ImageView: {}", i), m_Images[i].ImageView);
		}
		// Create command buffers
		{
			for (auto& commandBuffer : m_CommandBuffers)
				vkDestroyCommandPool(device, commandBuffer.CommandPool, nullptr);

			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = m_QueueNodeIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			m_CommandBuffers.resize(m_ImageCount);
			for (auto& commandBuffer : m_CommandBuffers)
			{
				VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandBuffer.CommandPool));

				commandBufferAllocateInfo.commandPool = commandBuffer.CommandPool;
				VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer.CommandBuffer));
			}
		}

		auto framesInFlight = Renderer::GetConfig().FramesInFlight;
		if (m_ImageAvailableSemaphores.size() != framesInFlight)
		{
			m_ImageAvailableSemaphores.resize(framesInFlight);
			m_RenderFinishedSemaphores.resize(framesInFlight);
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for (size_t i = 0; i < framesInFlight; i++)
			{
				VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]));
				VKUtils::SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, fmt::format("Swapchain Semaphore ImageAvailable {0}", i), m_ImageAvailableSemaphores[i]);
				VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]));
				VKUtils::SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, fmt::format("Swapchain Semaphore RenderFinished {0}", i), m_RenderFinishedSemaphores[i]);
			}
		}

		if (m_WaitFences.size() != framesInFlight)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			m_WaitFences.resize(framesInFlight);
			for (auto& fence : m_WaitFences)
			{
				VK_CHECK_RESULT(vkCreateFence(m_Device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));
				VKUtils::SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, "Swapchain Fence", fence);
			}
		}

		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkFormat depthFormat = m_Device->GetPhysicalDevice()->GetDepthFormat();

		// Render Pass
		VkAttachmentDescription colorAttachmentDesc = {};
		// Color attachment
		colorAttachmentDesc.format = m_ColorFormat;
		colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// ������ͼ��û����
		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachmentDesc;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(m_Device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass));
		VKUtils::SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_RENDER_PASS, "Swapchain render pass", m_RenderPass);
		// Create framebuffers for every swapchain image
		{
			for (auto& framebuffer : m_Framebuffers)
				vkDestroyFramebuffer(device, framebuffer, nullptr);

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCreateInfo.renderPass = m_RenderPass;
			frameBufferCreateInfo.attachmentCount = 1;
			frameBufferCreateInfo.width = m_Width;
			frameBufferCreateInfo.height = m_Height;
			frameBufferCreateInfo.layers = 1;

			m_Framebuffers.resize(m_ImageCount);
			for (uint32_t i = 0; i < m_Framebuffers.size(); i++)
			{
				frameBufferCreateInfo.pAttachments = &m_Images[i].ImageView;
				VK_CHECK_RESULT(vkCreateFramebuffer(m_Device->GetVulkanDevice(), &frameBufferCreateInfo, nullptr, &m_Framebuffers[i]));
				VKUtils::SetDebugUtilsObjectName(m_Device->GetVulkanDevice(), VK_OBJECT_TYPE_FRAMEBUFFER, fmt::format("Swapchain framebuffer (Frame in flight: {})", i), m_Framebuffers[i]);
			}
		}
	}

	void VulkanSwapChain::Destroy()
	{
		HZ_CORE_WARN_TAG("Renderer", "VulkanSwapChain::OnDestroy");

		auto device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		if (m_SwapChain)
			fpDestroySwapchainKHR(device, m_SwapChain, nullptr);

		for (auto& image : m_Images)
			vkDestroyImageView(device, image.ImageView, nullptr);
		m_Images.clear();

		for (auto& commandBuffer : m_CommandBuffers)
			vkDestroyCommandPool(device, commandBuffer.CommandPool, nullptr);
		m_CommandBuffers.clear();

		if (m_RenderPass)
			vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		m_Framebuffers.clear();

		for (auto& semaphore : m_ImageAvailableSemaphores)
			vkDestroySemaphore(device, semaphore, nullptr);
		m_ImageAvailableSemaphores.clear();

		for (auto& semaphore : m_RenderFinishedSemaphores)
			vkDestroySemaphore(device, semaphore, nullptr);
		m_RenderFinishedSemaphores.clear();

		for (auto& fence : m_WaitFences)
			vkDestroyFence(device, fence, nullptr);
		m_WaitFences.clear();

		vkDeviceWaitIdle(device);
	}

	void VulkanSwapChain::OnResize(uint32_t width, uint32_t height)
	{
		auto device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);
		Create(&width, &height, m_VSync);
		vkDeviceWaitIdle(device);

	}

	void VulkanSwapChain::FindImageFormatAndColorSpace()
	{
		VkPhysicalDevice physicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();

		// Get list of supported surface formats
		uint32_t formatCount;
		VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, NULL));
		HZ_CORE_ASSERT(formatCount > 0);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data()));

		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			m_ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			bool found_B8G8R8A8_UNORM = false;
			for (auto&& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					m_ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;   // �������Եģ� ����50���Զ����н�������ɫ��ʽ
					m_ColorSpace = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}

			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if (!found_B8G8R8A8_UNORM)
			{
				m_ColorFormat = surfaceFormats[0].format;
				m_ColorSpace = surfaceFormats[0].colorSpace;
			}
		}

	}

	uint32_t VulkanSwapChain::AcquireNextImage()
	{
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;


		// Make sure the frame we're requesting has finished rendering (from previous iterations)
		{
			HZ_PROFILE_SCOPE("VulkanSwapChain::AcquireNextImage - WaitForFences");
			VK_CHECK_RESULT(vkWaitForFences(m_Device->GetVulkanDevice(), 1, &m_WaitFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX));
		}

		uint32_t imageIndex;
		VkResult result = fpAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex], (VkFence)nullptr, &imageIndex);
		if (result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				OnResize(m_Width, m_Height);
				VK_CHECK_RESULT(fpAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex], (VkFence)nullptr, &imageIndex));
			}
		}

		return imageIndex;
	}
	/*void VulkanSwapChain::createFinalColorPipeline()
	{
		// ����Shader
		Shader::DescriptorBinding finalColorSamplerBinding;
		finalColorSamplerBinding.binding = 0;
		finalColorSamplerBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		finalColorSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		finalColorSamplerBinding.count = 1;
		Shader::ShaderSpecification finalColorSpec;
		finalColorSpec.bindings.push_back(finalColorSamplerBinding);
		shader = new VulkanShader("finalColor", "assets/shaders/Debug/vert.spv", "assets/shaders/Debug/frag.spv", finalColorSpec,m_Device->GetVulkanDevice());
		auto device = m_Device->GetVulkanDevice();

		// Shader��Ϣ
		VkShaderModule vertShaderModule = shader->GetVertShaderModule();
		VkShaderModule fragShaderModule = shader->GetFragShaderModule();
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// ������Ϣ
		// ������Ϣ���ؼ��޸ģ������ⲿ��������
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// ���������Ǻ����޸ģ���ʹ���κζ���󶨺�����
		vertexInputInfo.vertexBindingDescriptionCount = 0;  // �޶����
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;  // �޶�������
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;


		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;  // ����ָ����������������֣���shader��ͨ��layout(set = x)ָ��ʹ���ĸ�
		pipelineLayoutInfo.pSetLayouts = shader->GetDescriptorSetLayout();

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = GetRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		//vkDestroyShaderModule(device, fragShaderModule, nullptr);
		//vkDestroyShaderModule(device, vertShaderModule, nullptr);
	//}
}*/
}
