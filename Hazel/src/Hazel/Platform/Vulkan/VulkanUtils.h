#pragma once
#include "Vulkan.h"
#include <optional>
#include <vector>
#include <string>
#include "Hazel/Renderer/Renderer.h"
inline PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT; //Making it static randomly sets it to nullptr for some reason.
inline PFN_vkCmdBeginDebugUtilsLabelEXT fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT fpCmdInsertDebugUtilsLabelEXT;
namespace Hazel {
	namespace VKUtils {
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
		void SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const std::string& name, const void* handle);
		VkResult CreateDebugUtilsMessengerEXT(VkInstance m_Instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void VulkanLoadDebugUtilsExtensions(VkInstance m_Instance,VkDevice device);
		bool CheckDriverAPIVersionSupport(uint32_t minimumSupportedVersion);

	}
	namespace Utils {
		inline void DumpGPUInfo()
		{
			auto& caps = Renderer::GetCapabilities();
			LOG_TRACE_TAG("Renderer", "GPU info:");
			LOG_TRACE_TAG("Renderer", "  Vendor: {0}", caps.Vendor);
			LOG_TRACE_TAG("Renderer", "  Device: {0}", caps.Device);
			LOG_TRACE_TAG("Renderer", "  Version: {0}", caps.Version);
		}
		inline void VulkanCheckResult(VkResult result)
		{
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("VkResult is '1'");
				if (result == VK_ERROR_DEVICE_LOST)
				{
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(3s);
					//Utils::RetrieveDiagnosticCheckpoints();
					Utils::DumpGPUInfo();
				}
				ASSERT(result == VK_SUCCESS);
			}
		}
	}
}

