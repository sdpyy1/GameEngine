#pragma once
#include "PipelineCompute.h"

namespace Hazel
{
	struct ComputePassSpecification
	{
		Ref<PipelineCompute> Pipeline;
		std::string DebugName;
		glm::vec4 MarkerColor;
	};
	class ComputePass : public RefCounted
	{
	public:
		virtual ~ComputePass() = default;
		virtual ComputePassSpecification& GetSpecification() = 0;
		virtual const ComputePassSpecification& GetSpecification() const = 0;
		virtual Ref<Shader> GetShader() const = 0;
		virtual Ref<Image2D> GetOutput(uint32_t index) = 0;
		virtual Ref<Image2D> GetDepthOutput() = 0;
		virtual Ref<PipelineCompute> GetPipeline() const = 0;
		virtual void SetInput(Ref<Image2D> image, uint32_t Binding) = 0;
		virtual void SetInput(Ref<ImageView> imageView, uint32_t Binding) = 0;

		virtual bool HasDescriptorSets() const = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);

	};
}

