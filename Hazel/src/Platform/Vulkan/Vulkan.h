#pragma once
#include <vulkan/vulkan.h>
namespace Hazel {
// TODO: Vulkan´íÎó¼ì²é
#define VK_CHECK_RESULT(f) f

// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
	HZ_CORE_ASSERT(fp##entrypoint);                                     \
}

// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
	fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint));   \
	HZ_CORE_ASSERT(fp##entrypoint);                                     \
}

}
