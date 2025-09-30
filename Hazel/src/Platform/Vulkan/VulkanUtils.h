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

#ifdef HZ_DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
	// —È÷§≤„
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	namespace VkUtils {
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
		void SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const std::string& name, const void* handle);
		std::vector<const char*> getRequiredExtensions();
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void VulkanLoadDebugUtilsExtensions(VkInstance instance);
		bool CheckDriverAPIVersionSupport(uint32_t minimumSupportedVersion);
		bool checkValidationLayerSupport();

	}
}

