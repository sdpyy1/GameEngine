#pragma once
#include "Vulkan.h"
#include <map>

namespace Hazel {
	class VulkanPhysicalDevice {
	public:
		struct QueueFamilyIndices
		{
			int32_t Graphics = -1;
			int32_t Compute = -1;
			int32_t Transfer = -1;
		};
	public:
		VulkanPhysicalDevice();
		~VulkanPhysicalDevice();
		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		QueueFamilyIndices GetQueueFamilyIndices(int queueFlags);
		bool IsExtensionSupported(const std::string& extensionName) const;
		VkFormat GetDepthFormat() const { return m_DepthFormat; }
		std::string getDeviceName() { return std::string(m_Properties.deviceName); }
		static Ref<VulkanPhysicalDevice> Select(); // Create Static PhysicalDevice

	private:
		VkFormat FindDepthFormat() const;
	private:
		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceFeatures m_Features;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties;
		std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
		std::unordered_set<std::string> m_SupportedExtensions;
		std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;
		QueueFamilyIndices m_QueueFamilyIndices;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;

		friend class VulkanDevice;

	};

	class VulkanDevice {

	public:
		VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~VulkanDevice();
		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetComputeQueue() { return m_ComputeQueue; }
		VkQueue GetTransferQueue() { return m_TransferQueue; }
		const Ref<VulkanPhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		static Ref<VulkanDevice> Create(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures); // Create Static Device
		VkDevice GetVulkanDevice() const { return m_LogicalDevice; }

	private:
		VkDevice m_LogicalDevice = nullptr;
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		VkPhysicalDeviceFeatures m_EnabledFeatures;

		VkQueue m_GraphicsQueue;
		VkQueue m_ComputeQueue;
		VkQueue m_TransferQueue;

		//std::map<std::thread::id, Ref<VulkanCommandPool>> m_CommandPools;
		bool m_EnableDebugMarkers = false;

		std::mutex m_GraphicsQueueMutex, m_ComputeQueueMutex;
	};
}

