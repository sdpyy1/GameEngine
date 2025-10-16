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
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}


	void VulkanRenderPass::SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = UboSet->Get(i).As<VulkanUniformBuffer>()->GetVkBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = UboSet.As<VulkanUniformBufferSet>()->Get_PreSize();
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
			descriptorWrites[0].dstBinding = Binding; 
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &UboSet->Get(i).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo(); 

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
	void VulkanRenderPass::SetInput(Ref<Texture2D> texture, uint32_t Binding)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		auto vulkanTexture = texture.As<VulkanTexture2D>();

		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;         
			imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;      
			imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout; 

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
			descriptorWrites[0].dstBinding = Binding;
			descriptorWrites[0].dstArrayElement = 0; 
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
			descriptorWrites[0].descriptorCount = 1; 
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	Ref<Image2D> VulkanRenderPass::GetDepthOutput()
	{
		Ref<Framebuffer> framebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		if (!framebuffer->HasDepthAttachment())
			return nullptr; // No depth output
		return framebuffer->GetDepthImage();
	}

	void VulkanRenderPass::SetInput(Ref<Image2D> image, uint32_t Binding,bool isInit)
	{
		Renderer::Submit([=]() {
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = image.As<VulkanImage2D>();

			if (isInit) {
				for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
					VkDescriptorImageInfo imageInfo{};
					imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;
					imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;
					imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;

					std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
					descriptorWrites[0].dstBinding = Binding;
					descriptorWrites[0].dstArrayElement = 0;
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[0].descriptorCount = 1;
					descriptorWrites[0].pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
			}else {
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
			}
		});
	}
}
