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
	}

	void VulkanMaterial::UpdateDescriptorSet() {
		// 1. 选择当前要绑定的贴图（优先用户设置，否则用默认）
		auto albedo = AlbedoTexture;
		auto normal = NormalTexture;
		auto metalness =MetalnessTexture;
		auto roughness = RoughnessTexture;

		// 2. 准备贴图的 ImageInfo（结合采样器，假设贴图自带采样器）
		auto getImageInfo = [](Ref<Texture2D> tex) -> VkDescriptorImageInfo {
			VkDescriptorImageInfo info{};
			info.sampler = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan().sampler; // 假设 Texture2D 类有 GetSampler 方法
			info.imageView = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan().imageView; // 假设 Texture2D 类有 GetImageView 方法
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // 贴图需提前转换到该布局
			return info;
			};

		std::array<VkDescriptorImageInfo, 4> imageInfos = {
			getImageInfo(albedo),    // binding 1: u_AlbedoTexture
			getImageInfo(normal),    // binding 2: u_NormalTexture
			getImageInfo(metalness), // binding 3: u_MetalnessTexture
			getImageInfo(roughness)  // binding 4: u_RoughnessTexture
		};

		// 3. 准备描述符写入操作（写入 Shader 的 Set = 0 的 binding 1~4）
		std::array<VkWriteDescriptorSet, 4> writeSets{};
		for (size_t i = 0; i < 4; i++) {
			writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSets[i].dstBinding = static_cast<uint32_t>(i) + 1; // binding = 1~4（与 Shader 对应）
			writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeSets[i].descriptorCount = 1;
			writeSets[i].pImageInfo = &imageInfos[i];
		}

		// 4. 为 Shader 中所有帧的 Set = 0 更新贴图（多帧缓冲时每个帧都要更新）
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		for (uint32_t frame = 0; frame < framesInFlight; frame++) {
			// 获取 Shader 的 Set = 0 描述符集（当前帧）
			VkDescriptorSet shaderSet = m_Shader->GetDescriptorSet()[frame];

			// 为当前帧的 Set 设置 dstSet
			for (auto& write : writeSets) {
				write.dstSet = shaderSet;
			}

			// 批量更新当前帧的 4 个 binding
			vkUpdateDescriptorSets(
				m_Device,
				static_cast<uint32_t>(writeSets.size()),
				writeSets.data(),
				0, nullptr
			);
		}
	}
	void VulkanMaterial::SetNormalTexture(Ref<Texture2D> texture)
	{
		NormalTexture = texture;
	}
	void VulkanMaterial::SetMetalnessTexture(Ref<Texture2D> texture)
	{
		MetalnessTexture = texture;
	}
	void VulkanMaterial::SetRoughnessTexture(Ref<Texture2D> texture)
	{
		RoughnessTexture = texture;
	}
	void VulkanMaterial::SetAlbedoTexture(Ref<Texture2D> texture)
	{
		AlbedoTexture = texture;
	}
	VulkanMaterial::~VulkanMaterial()
	{
	}
}
