#include "hzpch.h"
#include "VulkanMaterial.h"
#include <Hazel/Renderer/Shader.h>
#include "VulkanContext.h"
#include "VulkanTexture.h"
namespace Hazel {
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader.As<VulkanShader>()), m_Name(name), m_DescriptorManager(m_Shader)
	{
		m_Device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		if (m_Shader->GetComputeFilePath() == "")
		{
			// 旧接口
			CreateDescriptorSet();
			UpdateDescriptorSet(true);// 如果一个模型没有材质，也需要先初始化一次 DescriptorSet
		}
		else {
			m_DescriptorManager.SetManagedDescriptorSet(1);
		}

		
	}
	void VulkanMaterial::CreateDescriptorSet()
	{
		Renderer::Submit([this] {
			m_DescriptorSets = m_Shader->createDescriptorSet(1);
			});
	}
	void VulkanMaterial::UpdateDescriptorSet(bool bInit)
	{
		Renderer::Submit([this, bInit] {
			// 选择贴图（用户设置或默认）
			auto albedo = AlbedoTexture ? AlbedoTexture : Renderer::GetWhiteTexture();
			auto normal = NormalTexture ? NormalTexture : Renderer::GetWhiteTexture();
			auto metalness = MetalnessTexture ? MetalnessTexture : Renderer::GetWhiteTexture();
			auto roughness = RoughnessTexture ? RoughnessTexture : Renderer::GetWhiteTexture();
			auto rms = EmissionTexture ? EmissionTexture : Renderer::GetBlackTexture();

			// 准备 VkDescriptorImageInfo
			auto getImageInfo = [](Ref<Texture2D> tex) -> VkDescriptorImageInfo {
				VkDescriptorImageInfo info{};
				auto descriptor = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan();
				info.sampler = descriptor.sampler;
				info.imageView = descriptor.imageView;
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				return info;
				};

			std::array<VkDescriptorImageInfo, 5> imageInfos = {
				getImageInfo(albedo),
				getImageInfo(normal),
				getImageInfo(metalness),
				getImageInfo(roughness),
                getImageInfo(rms)
			};

			// 写入操作模板
			std::array<VkWriteDescriptorSet, 5> writeSets{};
			for (size_t i = 0; i < writeSets.size(); i++)
			{
				writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeSets[i].dstBinding = static_cast<uint32_t>(i); // binding 0~3
				writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeSets[i].descriptorCount = 1;
				writeSets[i].pImageInfo = &imageInfos[i];
			}

			if (bInit)
			{
				// 初始化：更新所有帧的 DescriptorSet
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
				{
					for (auto& write : writeSets)
						write.dstSet = m_DescriptorSets[frameIndex];

					vkUpdateDescriptorSets(
						m_Device,
						static_cast<uint32_t>(writeSets.size()),
						writeSets.data(),
						0, nullptr
					);
				}
			}
			else
			{
				// 平时渲染：只更新当前帧
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkDescriptorSet currentSet = m_DescriptorSets[frameIndex];
				for (auto& write : writeSets)
					write.dstSet = currentSet;

				vkUpdateDescriptorSets(
					m_Device,
					static_cast<uint32_t>(writeSets.size()),
					writeSets.data(),
					0, nullptr
				);
			}
			});
	}

	void VulkanMaterial::SetNormalTexture(Ref<Texture2D> texture)
	{
		NormalTexture = texture;
		UpdateDescriptorSet(true);
	}
	void VulkanMaterial::SetMetalnessTexture(Ref<Texture2D> texture)
	{
		MetalnessTexture = texture;
		UpdateDescriptorSet(true);
	}
	void VulkanMaterial::SetRoughnessTexture(Ref<Texture2D> texture)
	{
		RoughnessTexture = texture;
		UpdateDescriptorSet(true);
	}
	void VulkanMaterial::SetEmissionTexture(Ref<Texture2D> texture)
	{
		EmissionTexture = texture;
		UpdateDescriptorSet(true);
	}
	void VulkanMaterial::SetAlbedoColor(glm::vec3 color)
	{
		m_AlbedoColor = color;
	}
	void VulkanMaterial::SetMetalnessColor(float color)
	{
		m_MetalnessColor = color;
	}
	void VulkanMaterial::SetRoughnessColor(float color)
	{
		m_RoughnessColor = color;
	}
	void VulkanMaterial::SetEmissionColor(glm::vec3 color)
	{
		m_EmissionColor = color;
	}

	void VulkanMaterial::SetAlbedoTexture(Ref<Texture2D> texture)
	{
		AlbedoTexture = texture;
		UpdateDescriptorSet(true);
	}
	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::SetInput(std::string name, Ref<UniformBufferSet> UboSet)
	{
		m_DescriptorManager.SetInput2Addition(name,UboSet);
	}

	void VulkanMaterial::SetInput(std::string name, Ref<Texture2D> texture, bool isInit)
	{
		m_DescriptorManager.SetInput2Addition(name, texture, isInit);
	}
	void VulkanMaterial::SetInput(std::string name, Ref<ImageView> texture)
	{
		m_DescriptorManager.SetInput2Addition(name, texture);
	}
	void VulkanMaterial::SetInput(std::string name, Ref<StorageBufferSet> SBSet)
	{
		m_DescriptorManager.SetInput2Addition(name, SBSet);
	}

	void VulkanMaterial::SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit)
	{
		m_DescriptorManager.SetInput2Addition(name, cubeMap, isInit);
	}

	void VulkanMaterial::SetInput(std::string name, Ref<Image2D> image, bool isInit)
	{
		m_DescriptorManager.SetInput2Addition(name, image, isInit);
	}

}
