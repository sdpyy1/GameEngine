#pragma once
#include "VulkanShader.h"
#include "Hazel/Renderer/PipelineCompute.h"
namespace Hazel
{
	class VulkanComputePipeline : public PipelineCompute 
	{
    public:
		VulkanComputePipeline(Ref<Shader> computeShader);
		void RT_CreatePipeline();
		void RT_Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) override;
		Ref<Shader> GetShader() const override { return m_Shader; }
		void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) override;
		void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess) override;
		void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess)override;
		void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) override;
		void Dispatch(const glm::uvec3& workGroups) const;
		void End() override;
		VkPipelineLayout GetLayout() const { return m_ComputePipelineLayout; }
		void SetPushConstants(Buffer constants) const;

	private:
		Ref<VulkanShader> m_Shader;
		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;
		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;
		bool m_UsingGraphicsQueue = false;

	};

}
