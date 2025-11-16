#pragma once
#define VK_NO_PROTOTYPES  // volk
#include <vulkan/vulkan.h>

#include "Hazel/Renderer/RHI/RHI.h"

namespace Hazel
{
	class VulkanDynamicRHI : public DynamicRHI
	{
	public:
		VulkanDynamicRHI() = delete;
		VulkanDynamicRHI(const RHIConfig& config);


	private:
		inline VkInstance GetInstance() const { return m_Instance; }

		void CreateInstance();
		void CreatePhysicalDevice();
		void CreateLogicalDevice();
		void CreateQueues();







	private:
		// 实例
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		std::vector<VkLayerProperties> m_AvailableLayers;

		// 物理设备
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
		VkPhysicalDeviceSamplerFilterMinmaxProperties filterMinmaxProperties;           //采样器特性
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_PhysicalDeviceRayTracingPipelineProperties;   //光追特性
		std::vector<std::string> m_PhysicalDeviceSupportedExtensions;
		// 逻辑设备
		VkDevice m_LogicalDevice;

		// 队列
		std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties; // 所有队列族
		std::array<int32_t, QUEUE_TYPE_MAX_ENUM> m_QueueIndices; // 每种类型的队列来自哪一个队列族
		std::array<std::array<RHIQueueRef, MAX_QUEUE_CNT>, QUEUE_TYPE_MAX_ENUM> m_Queues; //预创建的所有队列

	};





}
