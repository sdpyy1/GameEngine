#pragma once

#include "vulkan/vulkan.h"
#include "VulkanAllocator.h"

#include <string>
#include <unordered_map>

namespace Hazel {
	// 定义反射信息
	namespace ShaderResource {

		struct UniformBuffer
		{
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct StorageBuffer
		{
			VmaAllocation MemoryAlloc = nullptr;
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct ImageSampler
		{
			uint32_t BindingPoint = 0;
			uint32_t DescriptorSet = 0;
			uint32_t Dimension = 0;
			uint32_t ArraySize = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct PushConstantRange
		{
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t Offset = 0;
			uint32_t Size = 0;
		};


		// Binding -> Descriptor
		struct ShaderDescriptorSet 
		{
			std::unordered_map<uint32_t, UniformBuffer> UniformBuffers;
			std::unordered_map<uint32_t, StorageBuffer> StorageBuffers;
			std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
			std::unordered_map<uint32_t, ImageSampler> StorageImages;
			std::unordered_map<uint32_t, ImageSampler> SeparateTextures; // Not really an image sampler.
			std::unordered_map<uint32_t, ImageSampler> SeparateSamplers;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

			operator bool() const { return !(StorageBuffers.empty() && UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty() && SeparateTextures.empty() && SeparateSamplers.empty()); }
		};

	}
}
