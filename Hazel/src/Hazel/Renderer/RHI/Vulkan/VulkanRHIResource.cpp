#include "hzpch.h"
#include "VulkanRHIResource.h"
#include "VulkanRHI.h"
#include "VulkanUtil.h"
namespace Hazel
{

	VulkanRHISurface::VulkanRHISurface(GLFWwindow* window)
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		extent = { (uint32_t)width, (uint32_t)height };
		VK_CHECK_RESULT(glfwCreateWindowSurface(VULKAN_INSTANCE, window, nullptr, &handle));
	}

	void VulkanRHISurface::Destroy()
	{
		vkDestroySurfaceKHR(VULKAN_INSTANCE, handle, nullptr);
	}

    VulkanRHISwapchain::VulkanRHISwapchain(const RHISwapchainInfo& info): RHISwapchain(info)
    {
        VkPhysicalDevice device = VULKAN_PHYSICALDEVICE;
        VkDevice logicalDevice = VULKAN_DEVICE;
        VkSurfaceKHR surface = std::static_pointer_cast<VulkanRHISurface>(info.surface)->GetHandle();

        // 设备支持信息
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

        uint32_t size;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, nullptr);
        availableFormats.resize(size);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &size, availableFormats.data());

        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, nullptr);
        availablePresentModes.resize(size);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &size, availablePresentModes.data());

        // 交换链基本信息
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(VulkanUtil::RHIFormatToVkFormat(info.format));
        RHIFormat targetFormat = VulkanUtil::VkFormatToRHIFormat(surfaceFormat.format);
        if (targetFormat != info.format)
        {
            this->info.format = targetFormat;
            LOG_ERROR("Cant find swapchain image format support!");
        }

        VkPresentModeKHR presentMode = ChooseSwapPresentMode();

        VkExtent2D extent = ChooseSwapExtent();
        if (extent.width != info.extent.width || extent.height != info.extent.height)
        {
            this->info.extent = { extent.width, extent.height };
            LOG_ERROR("Cant find suitable swapchain image extent!");
        }

        // 交换链图像数目
        uint32_t imageCount = std::max(info.imageCount, capabilities.minImageCount);
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }
        if (info.imageCount != imageCount)
        {
            this->info.imageCount = capabilities.maxImageCount;
            LOG_ERROR("Swapchain image count is greater than capability maximum!");
        }

        // 创建交换链信息
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT;

        // 检测队列族对交换链图像的操作方式
        /*
        uint32_t queueFamilyIndices[] = { (uint32_t)queueInfo.graphicsFamily, (uint32_t)queueInfo.presentFamily };
        if (queueInfo.graphicsFamily != queueInfo.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   // 图像可被多个队列族访问
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // 图像同一时间只能被单个队列族访问
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        */
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;    // 图像同一时间只能被单个队列族访问
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional

        createInfo.preTransform = capabilities.currentTransform;                    // 变换操作，例如旋转反转，用默认
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // 透明度混合
        createInfo.presentMode = presentMode;                                       // 刷新模式
        createInfo.clipped = VK_TRUE;                                               // 裁剪（遮挡或再可视范围外等）
        createInfo.oldSwapchain = VK_NULL_HANDLE;                                   // 交换链更新时使用

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, nullptr);   // 获取image句柄
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, images.data());

        imageFormat = surfaceFormat.format;  // 存储extent和format
        imageExtent = extent;

        for (uint32_t i = 0; i < imageCount; i++)
        {
            RHITextureInfo info = {
                info.format = targetFormat,
                info.extent = { extent.width, extent.height, 1},
                info.arrayLayers = 1,
                info.mipLevels = 1,
                info.memoryUsage = MEMORY_USAGE_GPU_ONLY,
                info.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RENDER_TARGET,
                info.creationFlag = TEXTURE_CREATION_NONE
            };

            RHITextureRef texture = std::make_shared<VulkanRHITexture>(info, images[i]);
            textures.push_back(texture);

            // 留着RESOURCE_STATE_UNDEFINED之后处理其实也可以，可加可不加
            /*backend.GetImmediateCommand()->TextureBarrier(
                { texture, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_PRESENT, {TEXTURE_ASPECT_COLOR, 0, 1, 0, 1} });
            backend.GetImmediateCommand()->Flush();*/
        }
    }

	Hazel::RHITextureRef VulkanRHISwapchain::GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore)
	{
        VkFence signalFence = VK_NULL_HANDLE;
        VkSemaphore semaphore = VK_NULL_HANDLE;

        if (fence != nullptr) signalFence = std::static_pointer_cast<VulkanRHIFence>(fence)->GetHandle();
        if (signalSemaphore != nullptr) semaphore = std::static_pointer_cast<VulkanRHISemaphore>(signalSemaphore)->GetHandle();

        VkResult result = vkAcquireNextImageKHR(VULKAN_DEVICE,handle, UINT64_MAX, semaphore, signalFence, &currentIndex);

        return textures[currentIndex];
	}

    VkSurfaceFormatKHR VulkanRHISwapchain::ChooseSwapSurfaceFormat(VkFormat targetFormat)
    {
        // 选择通道标准，以及色彩空间
        std::cout << "Available swapchain surface formats:" << std::endl;
        for (const auto format : availableFormats) {
            std::cout << format.format << " : " << format.colorSpace << std::endl;
        }
        std::cout << " " << std::endl;

        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {    //无偏向性，任选
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == targetFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanRHISwapchain::ChooseSwapPresentMode()
    {
        // 选择刷新模式
        std::cout << "Available swapchain present modes:" << std::endl;
        for (const auto mode : availablePresentModes) {
            std::cout << mode << std::endl;
        }
        std::cout << " " << std::endl;

        VkPresentModeKHR bestMode;
        bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;         // normal
        //bestMode = VK_PRESENT_MODE_MAILBOX_KHR;             // low latency
        //bestMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;      // minimize stuttering
        //bestMode = VK_PRESENT_MODE_FIFO_KHR;              // low power consumption

        // for (const auto& availablePresentMode : presentModes) {
        //     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        //         return availablePresentMode;
        //     }
        //     else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        //         bestMode = availablePresentMode;
        //     }
        // }

        return bestMode;
    }

    VkExtent2D VulkanRHISwapchain::ChooseSwapExtent()
    {
        // 选择分辨率
        std::cout << "Swapchain extent: " << capabilities.currentExtent.width << " : " << capabilities.currentExtent.height << std::endl;

        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else {
            //int width, height;
            //glfwGetWindowSize(pWindow, &width, &height);
            VkExtent2D actualExtent = { this->info.extent.width, this->info.extent.height };

            //std::cout << width << " " << height << std::endl;

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

	void VulkanRHISwapchain::Present(RHISemaphoreRef waitSemaphore)
	{
        VkSemaphore semaphore = VK_NULL_HANDLE;
        if (waitSemaphore != nullptr) semaphore = std::static_pointer_cast<VulkanRHISemaphore>(waitSemaphore)->GetHandle();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &handle;
        presentInfo.pImageIndices = &currentIndex;
        presentInfo.pResults = nullptr;
        presentInfo.waitSemaphoreCount = semaphore == VK_NULL_HANDLE ? 0 : 1;
        presentInfo.pWaitSemaphores = &semaphore;

        if (vkQueuePresentKHR(std::static_pointer_cast<VulkanRHIQueue>(this->info.presentQueue)->GetHandle(), &presentInfo) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to present swap chain image!\n");
        }
	}

	void VulkanRHISwapchain::Destroy()
	{
        vkDestroySwapchainKHR(VULKAN_DEVICE, handle, nullptr);
	}

	VulkanRHITexture::VulkanRHITexture(const RHITextureInfo& info, VkImage image) : RHITexture(info)
	{
        TextureAspectFlags aspects = IsDepthStencilFormat(info.format) ? TEXTURE_ASPECT_DEPTH_STENCIL :IsDepthFormat(info.format) ? TEXTURE_ASPECT_DEPTH :IsStencilFormat(info.format) ? TEXTURE_ASPECT_STENCIL : TEXTURE_ASPECT_COLOR;
        defaultRange = { aspects, 0, info.mipLevels, 0, info.arrayLayers };
        defaultLayers = { aspects, 0, 0, info.arrayLayers };

        if (image != VK_NULL_HANDLE)
        {
            handle = image;
            return;
        }

        VkFormat format = VulkanUtil::RHIFormatToVkFormat(info.format);

        VkImageUsageFlags usage = VulkanUtil::ResourceTypeToImageUsage(info.type);
        if (IsDepthFormat(info.format) || IsStencilFormat(info.format))   
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        else if (info.type & RESOURCE_TYPE_RENDER_TARGET)                               
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


        VkImageType type = info.extent.depth > 1 ? VK_IMAGE_TYPE_3D :info.extent.height > 1 ? VK_IMAGE_TYPE_2D :VK_IMAGE_TYPE_1D;
        if (info.creationFlag & TEXTURE_CREATION_FORCE_2D) type = VK_IMAGE_TYPE_2D;
        if (info.creationFlag & TEXTURE_CREATION_FORCE_3D) type = VK_IMAGE_TYPE_3D;

        VkImageCreateFlags flag = 0;
        if (info.type & RESOURCE_TYPE_TEXTURE_CUBE)      flag |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        if (type == VK_IMAGE_TYPE_3D)                    flag |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;


        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = type;
        imageInfo.extent.width = info.extent.width;
        imageInfo.extent.height = info.extent.height;
        imageInfo.extent.depth = info.extent.depth;
        imageInfo.mipLevels = info.mipLevels;
        imageInfo.arrayLayers = info.arrayLayers;
        if (info.type & RESOURCE_TYPE_TEXTURE_CUBE) imageInfo.arrayLayers = std::max(imageInfo.arrayLayers, (uint32_t)6);
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = usage;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = flag; // Optional

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VulkanUtil::MemoryUsageToVma(info.memoryUsage);

        allocationInfo = {};
        if (vmaCreateImage(VULKAN_VMA, &imageInfo, &allocationCreateInfo, &handle, &allocation, &allocationInfo) != VK_SUCCESS)
        {
            LOG_ERROR("VMA failed to allocate image!");
        }
	}

	void VulkanRHITexture::Destroy()
	{
        vmaDestroyImage(VULKAN_VMA, handle, allocation);
	}


    VulkanRHIFence::VulkanRHIFence(bool signaled)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        vkCreateFence(VULKAN_DEVICE, &fenceInfo, nullptr, &handle);
    }

    void VulkanRHIFence::Wait()
    {
        vkWaitForFences(VULKAN_DEVICE, 1, &handle, VK_TRUE, UINT64_MAX);    //TODO 设置超时时间
        vkResetFences(VULKAN_DEVICE, 1, &handle);
    }

    void VulkanRHIFence::Destroy()
    {
        vkDestroyFence(VULKAN_DEVICE, handle, nullptr);
    }

    VulkanRHISemaphore::VulkanRHISemaphore()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(VULKAN_DEVICE, &semaphoreInfo, nullptr, &handle);
    }

    void VulkanRHISemaphore::Destroy()
    {
        vkDestroySemaphore(VULKAN_DEVICE, handle, nullptr);
    }

    VulkanRHICommandPool::VulkanRHICommandPool(const RHICommandPoolInfo& info)
        : RHICommandPool(info)
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = CAST<VulkanRHIQueue>(info.queue)->GetQueueFamilyIndex();  //命令池需要绑定队列族，使用其指定的命令类型
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

        if (vkCreateCommandPool(VULKAN_DEVICE, &poolInfo, nullptr, &handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create command pool!");
        }
    }

    void VulkanRHICommandPool::Destroy()
    {
        vkDestroyCommandPool(VULKAN_DEVICE, handle, nullptr);
    }

}
