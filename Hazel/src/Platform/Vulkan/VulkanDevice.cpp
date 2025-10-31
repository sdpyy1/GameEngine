#include "hzpch.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace Hazel
{
	VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance vkInstance)
	{
		uint32_t gpuCount = 0;
		// Get number of available physical devices
		vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr);
		HZ_CORE_ASSERT(gpuCount > 0, "No physical devices found!");
		// Enumerate devices
		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &gpuCount, physicalDevices.data()));
		VkPhysicalDevice selectedPhysicalDevice = nullptr;
		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			vkGetPhysicalDeviceProperties(physicalDevice, &m_Properties);
			if (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedPhysicalDevice = physicalDevice;
				break;
			}
		}

		if (!selectedPhysicalDevice)
		{
			HZ_CORE_INFO_TAG("Renderer", "Could not find discrete GPU. Using integrated GPU.");
			selectedPhysicalDevice = physicalDevices.back();
		}

		HZ_CORE_ASSERT(selectedPhysicalDevice, "Could not find any physical devices!");
		m_PhysicalDevice = selectedPhysicalDevice;

		// 扩展：查询基础特性和 Vulkan 1.2 特性
		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &vulkan12Features;

		vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);
		m_Features = deviceFeatures2.features; // 基础特性
		m_Vulkan12Features = vulkan12Features; // 保存 Vulkan 1.2 特性供后续使用

		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		HZ_CORE_ASSERT(queueFamilyCount > 0, "No queue families found!");
		m_QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (const auto& ext : extensions)
				{
					m_SupportedExtensions.emplace(ext.extensionName);
				}
			}
		}
		static const float defaultQueuePriority(0.0f);

		int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		m_QueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

		// Graphics queue
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			m_QueueCreateInfos.push_back(queueInfo);
		}

		// Dedicated compute queue
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
		{
			if (m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		// Dedicated transfer queue
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
		{
			if ((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				m_QueueCreateInfos.push_back(queueInfo);
			}
		}

		m_DepthFormat = FindDepthFormat();
		HZ_CORE_ASSERT(m_DepthFormat != VK_FORMAT_UNDEFINED, "No suitable depth format found!");
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{
	}

	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(int flags)
	{
		QueueFamilyIndices indices;
		indices.Graphics = -1;
		indices.Compute = -1;
		indices.Transfer = -1;

		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indices.Compute = i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.Transfer = i;
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indices.Compute = i;
			}

			if (flags & VK_QUEUE_GRAPHICS_BIT && indices.Graphics == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.Graphics = i;
			}
		}

		HZ_CORE_ASSERT(indices.Graphics != -1, "No graphics queue family found!");
		HZ_CORE_ASSERT(indices.Compute != -1, "No compute queue family found!");
		HZ_CORE_ASSERT(indices.Transfer != -1, "No transfer queue family found!");

		return indices;
	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
	{
		return m_SupportedExtensions.find(extensionName) != m_SupportedExtensions.end();
	}

	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select(VkInstance vkInstance)
	{
		return Ref<VulkanPhysicalDevice>::Create(vkInstance);
	}

	Ref<VulkanDevice> VulkanDevice::Create(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
	{
		return Ref<VulkanDevice>::Create(physicalDevice, enabledFeatures);
	}

	VkFormat VulkanPhysicalDevice::FindDepthFormat() const
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				return format;
		}
		return VK_FORMAT_UNDEFINED;
	}

	// 新增：获取 Vulkan 1.2 特性的接口
	const VkPhysicalDeviceVulkan12Features& VulkanPhysicalDevice::GetVulkan12Features() const
	{
		return m_Vulkan12Features;
	}

	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
		: m_PhysicalDevice(physicalDevice), m_EnabledFeatures(enabledFeatures)
	{
		const bool enableAftermath = true;

		// Do we need to enable any other extensions (eg. NV_RAYTRACING?)
		std::vector<const char*> deviceExtensions;
		// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
		HZ_CORE_ASSERT(m_PhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME), "Swapchain extension not supported!");
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		if (m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
			deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
		if (m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
			deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

#if HZ_HAS_AFTERMATH
		VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
		bool canEnableAftermath = enableAftermath && m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) && m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
		GpuCrashTracker* gpuCrashTracker = nullptr;
		if (canEnableAftermath)
		{
			// Must be initialized ~before~ device has been created
			gpuCrashTracker = hnew GpuCrashTracker();
			gpuCrashTracker->Initialize();

			VkDeviceDiagnosticsConfigFlagBitsNV aftermathFlags = (VkDeviceDiagnosticsConfigFlagBitsNV)(VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV);

			aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
			aftermathInfo.flags = aftermathFlags;
		}
#endif

		// 核心修改：配置特性链，启用 Vulkan 1.2 所需特性
		VkPhysicalDeviceVulkan12Features vulkan12Features = m_PhysicalDevice->GetVulkan12Features();
		// 显式启用 StorageImageArrayNonUniformIndexing 特性
		vulkan12Features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;

		// 构建特性链：基础特性 -> Vulkan 1.2 特性 -> Aftermath 信息（如果启用）
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features = enabledFeatures; // 基础启用的特性
		deviceFeatures2.pNext = &vulkan12Features;

#if HZ_HAS_AFTERMATH
		if (canEnableAftermath)
		{
			vulkan12Features.pNext = &aftermathInfo;
		}
#endif

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &deviceFeatures2; // 特性链关联到设备创建信息
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->m_QueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = physicalDevice->m_QueueCreateInfos.data();
		deviceCreateInfo.enabledExtensionCount = 0;
		deviceCreateInfo.ppEnabledExtensionNames = nullptr;
		// 注意：不再使用 pEnabledFeatures，而是通过 VkPhysicalDeviceFeatures2 传递特性

		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		{
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			m_EnableDebugMarkers = true;
		}

		if (!deviceExtensions.empty())
		{
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		VkResult result = vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice);
		HZ_CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device!");

		// Get a graphics queue from the device
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Transfer, 0, &m_TransferQueue);
	}

	VulkanDevice::~VulkanDevice()
	{
	}

	VulkanCommandPool::VulkanCommandPool()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &m_GraphicsCommandPool));

		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Compute;
		VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &m_ComputeCommandPool));
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		vkDestroyCommandPool(vulkanDevice, m_GraphicsCommandPool, nullptr);
		vkDestroyCommandPool(vulkanDevice, m_ComputeCommandPool, nullptr);
	}

	VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer(bool begin, bool compute)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = compute ? m_ComputeCommandPool : m_GraphicsCommandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice, &cmdBufAllocateInfo, &cmdBuffer));

		// If requested, also start the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}

	VkCommandBuffer VulkanDevice::GetCommandBuffer(bool begin, bool compute)
	{
		return GetOrCreateThreadLocalCommandPool()->AllocateCommandBuffer(begin, compute);
	}

	void VulkanCommandPool::FlushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		auto device = VulkanContext::GetCurrentDevice();
		FlushCommandBuffer(commandBuffer, device->GetGraphicsQueue());
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		GetThreadLocalCommandPool()->FlushCommandBuffer(commandBuffer);
	}

	void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
	{
		GetThreadLocalCommandPool()->FlushCommandBuffer(commandBuffer, queue);
	}

	void VulkanDevice::Destroy()
	{
		m_CommandPools.clear();
		vkDeviceWaitIdle(m_LogicalDevice);
		vkDestroyDevice(m_LogicalDevice, nullptr);
#if HZ_HAS_AFTERMATH
		// 清理 Aftermath 资源（如果需要）
		// 注意：原代码中 GpuCrashTracker 是 new 分配的，需要在这里释放
		// 请根据实际 Aftermath 集成逻辑调整
#endif
	}

	void VulkanDevice::LockQueue(bool compute)
	{
		if (compute)
			m_ComputeQueueMutex.lock();
		else
			m_GraphicsQueueMutex.lock();
	}

	void VulkanDevice::UnlockQueue(bool compute)
	{
		if (compute)
			m_ComputeQueueMutex.unlock();
		else
			m_GraphicsQueueMutex.unlock();
	}

	void VulkanCommandPool::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

		HZ_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "Command buffer is null!");

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence));

		{
			device->LockQueue();

			// Submit to the queue
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

			device->UnlockQueue();
		}
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(vulkanDevice, fence, nullptr);
		vkFreeCommandBuffers(vulkanDevice, m_GraphicsCommandPool, 1, &commandBuffer);
	}

	VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer(const char* debugName)
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = GetOrCreateThreadLocalCommandPool()->GetGraphicsCommandPool();
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		VKUtils::SetDebugUtilsObjectName(m_LogicalDevice, VK_OBJECT_TYPE_COMMAND_BUFFER, debugName, cmdBuffer);
		return cmdBuffer;
	}

	Ref<VulkanCommandPool> VulkanDevice::GetThreadLocalCommandPool()
	{
		auto threadID = std::this_thread::get_id();
		HZ_CORE_ASSERT(m_CommandPools.find(threadID) != m_CommandPools.end(), "Thread-local command pool not found!");

		return m_CommandPools.at(threadID);
	}

	Ref<VulkanCommandPool> VulkanDevice::GetOrCreateThreadLocalCommandPool()
	{
		// CommandPool与线程ID绑定，相当于一个线程一个CommandPool
		auto threadID = std::this_thread::get_id();
		auto commandPoolIt = m_CommandPools.find(threadID);
		if (commandPoolIt != m_CommandPools.end())
			return commandPoolIt->second;

		Ref<VulkanCommandPool> commandPool = Ref<VulkanCommandPool>::Create();
		m_CommandPools[threadID] = commandPool;
		return commandPool;
	}
}
