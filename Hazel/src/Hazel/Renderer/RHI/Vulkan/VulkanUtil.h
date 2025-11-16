#pragma once
#include "vulkan/vulkan.h"
#include <Hazel/Core/macro.h>
#include <GLFW/glfw3.h>
namespace Hazel {
#define VK_CHECK_RESULT(f)\
{\
	VkResult res = (f);\
	VulkanUtil::VulkanCheckResult(res, __FILE__, __LINE__);ASSERT(res == VK_SUCCESS);\
}
    static const char* INSTANCE_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation",
        //"VK_LAYER_RENDERDOC_Capture",  
    };
    static const char* DEVICE_LAYERS[] = {
        "废弃",
    };

    // TODO: 优先级
    static const float QUEUE_PRIORITIES[] = {
        1.f, 1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f, 1.f,
    };

    static const char* DEVICE_EXTENTIONS[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,          // 核心依赖扩展（实例级）
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    static const char* RAY_TRACING_DEVICE_EXTENTIONS[] = {
        // Ray tracing related extensions required
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,

        // Required by VK_KHR_acceleration_structure   
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

        // Required for VK_KHR_ray_tracing_pipeline
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,

        // Required by VK_KHR_spirv_1_4
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };
    // 验证层的功能
    static const std::vector<VkValidationFeatureEnableEXT> ENABLED_VALIDATION_FEATURES = {  // 需要时再启用，对帧数影响大
#if ENABLE_DEBUG_MODE
        //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
#endif
    };
    static const char* INSTANCE_EXTENTIONS[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    static const char* INSTANCE_VALIDATION_EXTENTIONS[] = {
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
    };
    static const char* TARGET_DEVICES[] = {
        "NVIDIA",
        "AMD"
    };
	namespace VulkanUtil {
        static bool IsExtensionSupported(const std::vector<std::string>& allExt, const std::string& extensionName)
        {
            for (const auto& ext : allExt) {
                if (ext == extensionName) {
                    return true;
                }
            }
            return false;
        }

        // 验证层Debug回调
        static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugCreateInfo.pfnUserCallback =
                [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData) -> VkBool32
                {
                    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                        LOG_ERROR_TAG("Vulkan Validation Error", pCallbackData->pMessage);
                    }
                    return VK_FALSE;
                };
            return debugCreateInfo;
        }

        // 验证层功能设置
        static VkValidationFeaturesEXT GetValidationFeatureCreateInfo()
        {
            VkValidationFeaturesEXT validationFeaturesExt = {};
            validationFeaturesExt.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            validationFeaturesExt.pEnabledValidationFeatures = ENABLED_VALIDATION_FEATURES.data();
            validationFeaturesExt.enabledValidationFeatureCount = ENABLED_VALIDATION_FEATURES.size();

            return validationFeaturesExt;
        }

        // 设置实例扩展
        static std::vector<const char*> GetRequiredInstanceExtensions(bool debug)
        {
            std::vector<const char*> extensions;

            unsigned int glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            for (unsigned int i = 0; i < glfwExtensionCount; i++) {
                extensions.push_back(glfwExtensions[i]);
            }

            for (auto extention : INSTANCE_EXTENTIONS)  extensions.push_back(extention);
            if (debug) for (auto extention : INSTANCE_VALIDATION_EXTENTIONS)  extensions.push_back(extention);

            return extensions;
        }


        static std::string QueueFlagsToString(VkQueueFlags queueFlags)
        {
            std::string str = "";
            if (queueFlags & VK_QUEUE_GRAPHICS_BIT)          str.append("VK_QUEUE_GRAPHICS_BIT ");
            if (queueFlags & VK_QUEUE_COMPUTE_BIT)           str.append("VK_QUEUE_COMPUTE_BIT ");
            if (queueFlags & VK_QUEUE_TRANSFER_BIT)          str.append("VK_QUEUE_TRANSFER_BIT ");
            if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)    str.append("VK_QUEUE_SPARSE_BINDING_BIT ");
            if (queueFlags & VK_QUEUE_PROTECTED_BIT)         str.append("VK_QUEUE_PROTECTED_BIT ");
            if (queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)  str.append("VK_QUEUE_VIDEO_DECODE_BIT_KHR ");
            if (queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)   str.append("VK_QUEUE_OPTICAL_FLOW_BIT_NV ");

            return  str;
        }


        inline const char* VKResultToString(VkResult result)
        {
            switch (result)
            {
            case VK_SUCCESS: return "VK_SUCCESS";
            case VK_NOT_READY: return "VK_NOT_READY";
            case VK_TIMEOUT: return "VK_TIMEOUT";
            case VK_EVENT_SET: return "VK_EVENT_SET";
            case VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
            case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
            case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
            case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
            case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
            case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
            case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
            case VK_PIPELINE_COMPILE_REQUIRED_EXT: return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
            }
            ASSERT(false);
            return nullptr;
        }
        inline void VulkanCheckResult(VkResult result, const char* file, int line)
        {
            if (result != VK_SUCCESS)
            {
                LOG_ERROR("VkResult is '{0}' in {1}:{2}", VKResultToString(result), file, line);
                if (result == VK_ERROR_DEVICE_LOST)
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(3s);

                }
            }
        }
        

	}
}