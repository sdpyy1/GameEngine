#pragma once
#include "Vulkan.h"
#include <map>
#include <unordered_set>

namespace Hazel {
	class VulkanPhysicalDevice : public RefCounted {
	public:
		struct QueueFamilyIndices
		{
			int32_t Graphics = -1;
			int32_t Compute = -1;
			int32_t Transfer = -1;
		};
	public:
		VulkanPhysicalDevice(VkInstance vkInstance);
		~VulkanPhysicalDevice();
		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceLimits& GetLimits() const { return m_Properties.limits; }

		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		QueueFamilyIndices GetQueueFamilyIndices(int queueFlags);
		bool IsExtensionSupported(const std::string& extensionName) const;
		VkFormat GetDepthFormat() const { return m_DepthFormat; }
		std::string getDeviceName() { return std::string(m_Properties.deviceName); }
		static Ref_old<VulkanPhysicalDevice> Select(VkInstance vkInstance); // Create Static PhysicalDevice
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }

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

	//  每个CommandPool都有图像和计算两个Pool
	class VulkanCommandPool : public RefCounted
	{
	public:
		VulkanCommandPool();
		virtual ~VulkanCommandPool();

		VkCommandBuffer AllocateCommandBuffer(bool begin, bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

		VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
		VkCommandPool GetComputeCommandPool() const { return m_ComputeCommandPool; }
	private:
		VkCommandPool m_GraphicsCommandPool, m_ComputeCommandPool;
	};
	class VulkanDevice : public RefCounted {

	public:
		VulkanDevice(const Ref_old<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~VulkanDevice();
		void Destroy();

		void LockQueue(bool compute = false);
		void UnlockQueue(bool compute = false);
		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetComputeQueue() { return m_ComputeQueue; }
		VkQueue GetTransferQueue() { return m_TransferQueue; }
		const Ref_old<VulkanPhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		static Ref_old<VulkanDevice> Create_old(const Ref_old<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures); // Create Static Device
		VkDevice GetVulkanDevice() const { return m_LogicalDevice; }
		VkCommandBuffer GetCommandBuffer(bool begin, bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);
		VkCommandBuffer CreateSecondaryCommandBuffer(const char* debugName);
	private:
		Ref<VulkanCommandPool> GetThreadLocalCommandPool();
		Ref<VulkanCommandPool> GetOrCreateThreadLocalCommandPool();
	private:
		VkDevice m_LogicalDevice = nullptr;
		Ref_old<VulkanPhysicalDevice> m_PhysicalDevice;
		VkPhysicalDeviceFeatures m_EnabledFeatures;

		VkQueue m_GraphicsQueue;
		VkQueue m_ComputeQueue;
		VkQueue m_TransferQueue;

		std::map<std::thread::id, Ref<VulkanCommandPool>> m_CommandPools;
		bool m_EnableDebugMarkers = false;

		std::mutex m_GraphicsQueueMutex, m_ComputeQueueMutex;
	};
}

