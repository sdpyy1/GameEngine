#include "hzpch.h"
#include "VulkanComputePass.h"
#include "VulkanShader.h"
#include <Hazel/Renderer/Texture.h>
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
#include "VulkanTexture.h"
namespace Hazel
{
	VulkanComputePass::VulkanComputePass(const ComputePassSpecification& spec) : m_Specification(spec)
	{
		HZ_CORE_VERIFY(spec.Pipeline);

	}

	Ref<Image2D> VulkanComputePass::GetOutput(uint32_t index)
	{
		HZ_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}

	Ref<Image2D> VulkanComputePass::GetDepthOutput()
	{
		HZ_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}	
	Ref<PipelineCompute> VulkanComputePass::GetPipeline() const
	{
		return m_Specification.Pipeline;
	}

	VulkanComputePass::~VulkanComputePass()
	{

	}

	bool VulkanComputePass::HasDescriptorSets() const
	{
		return m_Specification.Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet().size() > 0;
	}
	const VkDescriptorSet& VulkanComputePass::GetDescriptorSets(uint32_t frameIndex) const
	{
		return m_Specification.Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[frameIndex];
	}

	void VulkanComputePass::SetInput(Ref<Image2D> texture, uint32_t Binding)
	{
		Renderer::Submit([=]() {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput texture Binding {1}", m_Specification.DebugName, Binding);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = texture.As<VulkanImage2D>();

			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;
			imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;
			imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[Renderer::RT_GetCurrentFrameIndex()];
			descriptorWrites[0].dstBinding = Binding;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			});
	}

	void VulkanComputePass::SetInput(Ref<ImageView> imageView, uint32_t Binding)
	{
		Renderer::Submit([=]() {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput imageView[{2}] Binding {1}", m_Specification.DebugName, Binding, arrayIndex);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanImageView = imageView.As<VulkanImageView>();
			auto image = vulkanImageView->GetImage().As<VulkanImage2D>();
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = VK_NULL_HANDLE;
			imageInfo.imageView = vulkanImageView->GetImageView();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[Renderer::RT_GetCurrentFrameIndex()];
			descriptorWrites[0].dstBinding = Binding;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			});
	}
}
