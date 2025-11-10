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
	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec) :m_DescriptorManager(m_Specification.Pipeline->GetShader())
		,m_Specification(spec)
	{
		HZ_CORE_VERIFY(spec.Pipeline);  // RenderPass must have Pipeline Info
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}


	Ref<Image2D> VulkanRenderPass::GetDepthOutput()
	{
		Ref<Framebuffer> framebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		if (!framebuffer->HasDepthAttachment())
			return nullptr; // No depth output
		return framebuffer->GetDepthImage();
	}


	void VulkanRenderPass::SetInput(std::string name, Ref<UniformBufferSet> UboSet)
	{
		m_DescriptorManager.SetInput(name, UboSet);
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<Image2D> image, bool isInit)
	{
		m_DescriptorManager.SetInput(name, image, isInit);
	}
	void VulkanRenderPass::SetInput(std::string name, Ref<Texture2D> texture, bool isInit)
	{
		m_DescriptorManager.SetInput(name, texture, isInit);
	}


	void VulkanRenderPass::SetInput(std::string name, Ref<StorageBufferSet> SBSet)
	{
		m_DescriptorManager.SetInput(name, SBSet);
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit)
	{
		m_DescriptorManager.SetInput(name, cubeMap, isInit);
	}

}
