#include "hzpch.h"
#include "VulkanRenderPass.h"

#include "VulkanNativeCall.h"
#include "VulkanShader.h"
#include "VulkanContext.h"
#include "VulkanUniformBuffer.h"
#include "VulkanUniformBufferSet.h"
#include "VulkanStorageBuffer.h"
#include "VulkanStorageBufferSet.h"

#include "VulkanImage.h"
#include "VulkanTexture.h"

namespace Hazel {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		HZ_CORE_VERIFY(spec.Pipeline);  // RenderPass must have Pipeline Info

		DescriptorSetManagerSpecification dmSpec;
		dmSpec.DebugName = spec.DebugName;
		dmSpec.Shader = spec.Pipeline->GetSpecification().Shader.As<VulkanShader>();
		dmSpec.StartSet = 1;
		m_DescriptorSetManager = DescriptorSetManager(dmSpec);  // 从Shader信息中整理DescriptorSets如何设置
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}
}
