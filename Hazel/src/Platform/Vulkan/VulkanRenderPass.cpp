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
		m_BindImages.resize(Renderer::GetConfig().FramesInFlight);
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
		SetBindingKey setBinding = GetBinding(name);

		Renderer::Submit([setBinding, UboSet, this]() mutable {
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			uint32_t frameCount = Renderer::GetConfig().FramesInFlight;

			std::vector<VkWriteDescriptorSet> descriptorWrites;
			descriptorWrites.reserve(frameCount);

			std::vector<VkDescriptorBufferInfo> bufferInfos(frameCount);

			auto vkUboSet = UboSet.As<VulkanUniformBufferSet>();
			VkDeviceSize bufferRange = vkUboSet->Get_PreSize();

			for (uint32_t i = 0; i < frameCount; i++) {
				auto ubo = UboSet->Get(i).As<VulkanUniformBuffer>();

				bufferInfos[i].buffer = ubo->GetVkBuffer();
				bufferInfos[i].offset = 0;
				bufferInfos[i].range = bufferRange;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet(setBinding.set)[i];
				write.dstBinding = setBinding.binding;
				write.dstArrayElement = 0;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.descriptorCount = 1;
				write.pBufferInfo = &bufferInfos[i];

				descriptorWrites.push_back(write);
			}

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		});
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<Texture2D> texture, bool isInit)
	{
		SetBindingKey setBinding = GetBinding(name);

		Renderer::Submit([setBinding, texture, this, isInit]() {
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = texture.As<VulkanTexture2D>();

			const size_t framesInFlight = Renderer::GetConfig().FramesInFlight;

			if (isInit)
			{
				// 初始化阶段，更新所有帧
				std::vector<VkWriteDescriptorSet> descriptorWrites(framesInFlight);
				std::vector<VkDescriptorImageInfo> imageInfos(framesInFlight);

				for (size_t i = 0; i < framesInFlight; i++) {
					imageInfos[i] = vulkanTexture->GetDescriptorInfoVulkan();
					m_BindImages[i][setBinding] = vulkanTexture->GetImage().As<VulkanImage2D>()->GetVulkanImage();
					auto& write = descriptorWrites[i];
					write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[i];
					write.dstBinding = setBinding.binding;
					write.dstArrayElement = 0;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.descriptorCount = 1;
					write.pImageInfo = &imageInfos[i];
				}

				vkUpdateDescriptorSets(device,
					static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
					0, nullptr);
			}
			else
			{
				// 非初始化阶段，只更新当前帧
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkImage curImage = m_BindImages[frameIndex][setBinding];
				VkImage newImage = vulkanTexture->GetImage().As<VulkanImage2D>()->GetVulkanImage();
				VkDescriptorImageInfo imageInfo = vulkanTexture->GetDescriptorInfoVulkan();

				if (!curImage || curImage != newImage) {
					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[frameIndex];
					descriptorWrite.dstBinding = setBinding.binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
				}
			}
			});
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<Image2D> image, bool isInit)
	{
        SetBindingKey setBinding = GetBinding(name);
		Renderer::Submit([=]() {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput Image2D Binding {1}", m_Specification.DebugName, Binding);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = image.As<VulkanImage2D>();

			if (isInit) {
				uint32_t frames = Renderer::GetConfig().FramesInFlight;
				uint32_t layerCount = vulkanTexture->GetLayerCount();
				std::vector<VkWriteDescriptorSet> writes(frames);
				std::vector<std::vector<VkDescriptorImageInfo>> allImageInfos(frames);

				for (uint32_t i = 0; i < frames; ++i)
				{
					m_BindImages[i][setBinding] = vulkanTexture->GetVulkanImage();

					// 填充每帧的 imageInfo 数组
					allImageInfos[i].resize(layerCount);
					const auto& descInfo = vulkanTexture->GetDescriptorInfoVulkan();
					for (uint32_t l = 0; l < layerCount; ++l)
					{
						allImageInfos[i][l] =descInfo;
					}

					// 填写 VkWriteDescriptorSet
					writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writes[i].dstSet = GetVulkanShader()->GetDescriptorSet(0)[i];
					writes[i].dstBinding = setBinding.binding;
					writes[i].dstArrayElement = 0;
					writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[i].descriptorCount = layerCount;
					writes[i].pImageInfo = allImageInfos[i].data();
				}

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

			}
			else { 
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkImage curImage = m_BindImages[frameIndex][setBinding];
				VkImage newImage = image.As<VulkanImage2D>()->GetVulkanImage();

				if (!curImage || curImage != newImage)
				{
					m_BindImages[frameIndex][setBinding] = newImage;

					uint32_t layerCount = vulkanTexture->GetLayerCount();
					std::vector<VkDescriptorImageInfo> imageInfos(layerCount);
					const auto& descInfo = vulkanTexture->GetDescriptorInfoVulkan();

					for (uint32_t l = 0; l < layerCount; ++l)
					{
						imageInfos[l] = descInfo;
					}

					VkWriteDescriptorSet write{};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[frameIndex];
					write.dstBinding = setBinding.binding;
					write.dstArrayElement = 0;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.descriptorCount = layerCount;
					write.pImageInfo = imageInfos.data();

					vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
				}
			}
		});
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<StorageBufferSet> SBSet)
	{
        SetBindingKey setBinding = GetBinding(name);
		Renderer::Submit([setBinding,SBSet,this]() mutable{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
				auto vulkanBuffer = SBSet->Get(i).As<VulkanStorageBuffer>();
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = vulkanBuffer->GetVulkanBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = SBSet.As<VulkanStorageBufferSet>()->Get_PreSize();

				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[i];
				descriptorWrites[0].dstBinding = setBinding.binding; 
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		});
	}

	void VulkanRenderPass::SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit)
	{
		SetBindingKey setBinding = GetBinding(name);

		Renderer::Submit([=]() {
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = cubeMap.As<VulkanTextureCube>();

			VkDescriptorImageInfo imageInfo = vulkanTexture->GetDescriptorInfoVulkan();

			if (isInit)
			{
				size_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				std::vector<VkWriteDescriptorSet> descriptorWrites(framesInFlight);

				for (size_t i = 0; i < framesInFlight; i++)
				{
					m_BindImages[i][setBinding] = vulkanTexture->GetVulkanImage();

					VkWriteDescriptorSet& descriptorWrite = descriptorWrites[i]; // 直接引用数组元素
					descriptorWrite = {};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[i];
					descriptorWrite.dstBinding = setBinding.binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pImageInfo = &imageInfo;
				}
				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
			else
			{
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkImage curImage = m_BindImages[frameIndex][setBinding];
				VkImage newImage = vulkanTexture->GetVulkanImage();

				if (!curImage || curImage != newImage) {
					m_BindImages[frameIndex][setBinding] = newImage;
					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = GetVulkanShader()->GetDescriptorSet(setBinding.set)[frameIndex];
					descriptorWrite.dstBinding = setBinding.binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
				}
			}
			});
	}


	SetBindingKey VulkanRenderPass::GetBinding(std::string name) const
	{
		Ref<VulkanShader> shader = m_Specification.Pipeline->GetShader().As<VulkanShader>();
		return shader->getSetAndBinding(name);
	}

}
