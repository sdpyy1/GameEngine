#include "hzpch.h"
#include "VulkanUtils.h"
#include <GLFW/glfw3.h>
#include <set>

VKAPI_ATTR VkBool32 VKAPI_CALL Hazel::VKUtils::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl; return VK_FALSE;
}
void Hazel::VKUtils::SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const std::string& name, const void* handle)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo;
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.pObjectName = name.c_str();
	nameInfo.objectHandle = (uint64_t)handle;
	nameInfo.pNext = VK_NULL_HANDLE;

	VK_CHECK_RESULT(fpSetDebugUtilsObjectNameEXT(device, &nameInfo));
}
VkResult Hazel::VKUtils::CreateDebugUtilsMessengerEXT(VkInstance m_Instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(m_Instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
void Hazel::VKUtils::VulkanLoadDebugUtilsExtensions(VkInstance m_Instance,VkDevice device)
{
	fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
	if (fpSetDebugUtilsObjectNameEXT == nullptr)
		fpSetDebugUtilsObjectNameEXT = [](VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) { return VK_SUCCESS; };

	fpCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdBeginDebugUtilsLabelEXT"));
	if (fpCmdBeginDebugUtilsLabelEXT == nullptr)
		fpCmdBeginDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {};

	fpCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdEndDebugUtilsLabelEXT"));
	if (fpCmdEndDebugUtilsLabelEXT == nullptr)
		fpCmdEndDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer) {};

	fpCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdInsertDebugUtilsLabelEXT"));
	if (fpCmdInsertDebugUtilsLabelEXT == nullptr)
		fpCmdInsertDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {};
}

bool Hazel::VKUtils::CheckDriverAPIVersionSupport(uint32_t minimumSupportedVersion)
{
	uint32_t instanceVersion;
		vkEnumerateInstanceVersion(&instanceVersion);

		if (instanceVersion < minimumSupportedVersion)
		{
			HZ_CORE_CRITICAL("Incompatible Vulkan driver version!");
			HZ_CORE_CRITICAL("  You have {}.{}.{}", VK_API_VERSION_MAJOR(instanceVersion), VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
			HZ_CORE_CRITICAL("  You need at least {}.{}.{}", VK_API_VERSION_MAJOR(minimumSupportedVersion), VK_API_VERSION_MINOR(minimumSupportedVersion), VK_API_VERSION_PATCH(minimumSupportedVersion));
			return false;
		}
		return true;
}
