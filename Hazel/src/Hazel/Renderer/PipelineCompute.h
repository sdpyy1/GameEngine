#pragma once
#include "Hazel/Core/Ref.h"
#include "Shader.h"
#include "RenderCommandBuffer.h"
#include "StorageBuffer.h"


namespace Hazel
{
	class PipelineCompute : public RefCounted
	{
	public:
		static Ref<PipelineCompute> Create(Ref<Shader> computeShader);
		virtual void RT_Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) = 0;
		virtual void End() = 0;

		virtual void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess) = 0;
		virtual void BufferMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBuffer> storageBuffer, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) = 0;
		virtual void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, ResourceAccessFlags fromAccess, ResourceAccessFlags toAccess) = 0;
		virtual void ImageMemoryBarrier(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, PipelineStage fromStage, ResourceAccessFlags fromAccess, PipelineStage toStage, ResourceAccessFlags toAccess) = 0;
		virtual Ref<Shader> GetShader() const = 0;

	};

}
