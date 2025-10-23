#include "hzpch.h"
#include "VulkanMaterial.h"
#include <Hazel/Renderer/Shader.h>
#include "VulkanContext.h"
#include "VulkanTexture.h"
namespace Hazel {
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader.As<VulkanShader>()), m_Name(name)
	{
		m_Device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		CreateDescriptorSet();
		UpdateDescriptorSet(true);// ���һ��ģ��û�в��ʣ�Ҳ��Ҫ�ȳ�ʼ��һ�� DescriptorSet
	}
	void VulkanMaterial::CreateDescriptorSet()
	{
		Renderer::Submit([this] {
			VkDevice device = m_Device;
			VkDescriptorPool pool = m_Shader->GetDescriptorPool();       // Shader ר�� Pool
			VkDescriptorSetLayout layout = m_Shader->GetDescriptorSetLayout()[1]; // Set=1 ���ʲ���

			uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

			// ÿ֡������� DescriptorSet
			m_DescriptorSets.resize(framesInFlight);

			std::vector<VkDescriptorSetLayout> layouts(framesInFlight, layout);

			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = pool;
			allocInfo.descriptorSetCount = framesInFlight;
			allocInfo.pSetLayouts = layouts.data();

			VkResult result = vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data());
			assert(result == VK_SUCCESS && "Failed to allocate material DescriptorSets!");
		});
	}
	void VulkanMaterial::UpdateDescriptorSet(bool bInit)
	{
		Renderer::Submit([this,bInit] {
			// ѡ����ͼ���û����û�Ĭ�ϣ�
			auto albedo = AlbedoTexture ? AlbedoTexture : Renderer::GetWhiteTexture();
			auto normal = NormalTexture ? NormalTexture : Renderer::GetWhiteTexture();
			auto metalness = MetalnessTexture ? MetalnessTexture : Renderer::GetWhiteTexture();
			auto roughness = RoughnessTexture ? RoughnessTexture : Renderer::GetWhiteTexture();

			// ׼�� VkDescriptorImageInfo
			auto getImageInfo = [](Ref<Texture2D> tex) -> VkDescriptorImageInfo {
				VkDescriptorImageInfo info{};
				auto descriptor = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan();
				info.sampler = descriptor.sampler;
				info.imageView = descriptor.imageView;
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				return info;
				};

			std::array<VkDescriptorImageInfo, 4> imageInfos = {
				getImageInfo(albedo),
				getImageInfo(normal),
				getImageInfo(metalness),
				getImageInfo(roughness)
			};

			// д�����ģ��
			std::array<VkWriteDescriptorSet, 4> writeSets{};
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
				// ��ʼ������������֡�� DescriptorSet
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
				// ƽʱ��Ⱦ��ֻ���µ�ǰ֡
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
		Renderer::Submit([this, texture]() {
			NormalTexture = texture;
			UpdateDescriptorSet(true);
			});
	}
	void VulkanMaterial::SetMetalnessTexture(Ref<Texture2D> texture)
	{
		Renderer::Submit([this, texture]() {
			MetalnessTexture = texture;
			UpdateDescriptorSet(true);
			});
	}
	void VulkanMaterial::SetRoughnessTexture(Ref<Texture2D> texture)
	{
		Renderer::Submit([this, texture]() {
			RoughnessTexture = texture;
			UpdateDescriptorSet(true);
			});

	}
	void VulkanMaterial::SetAlbedoTexture(Ref<Texture2D> texture)
	{
		Renderer::Submit([this, texture]() {
			AlbedoTexture = texture;
			UpdateDescriptorSet(true);
			});

	}
	VulkanMaterial::~VulkanMaterial()
	{
	}
}
