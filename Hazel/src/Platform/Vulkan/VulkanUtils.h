#pragma once
#include "Vulkan.h"
#include <optional>
#include <vector>
#include <string>
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
}

