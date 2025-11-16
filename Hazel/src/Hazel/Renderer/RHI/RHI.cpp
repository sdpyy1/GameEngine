#include "hzpch.h"
#include "RHI.h"
#include "Vulkan/VulkanRHI.h"
namespace Hazel {
	DynamicRHIRef DynamicRHI::s_DynamicRHI = nullptr;
	DynamicRHIRef DynamicRHI::Init(RHIConfig config)
	{
		if (s_DynamicRHI == nullptr) {
			switch (config.api) {
			case API::Vulkan:
				s_DynamicRHI = std::make_shared<VulkanDynamicRHI>(config);
				break;
			default:
				LOG_ERROR("DynamicRHI not supported");
				break;
			}
		}
		return s_DynamicRHI;
	}
}