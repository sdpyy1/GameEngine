#pragma once
#include "PipelineCompute.h"
#include "Texture.h"
namespace Hazel
{
	struct ComputePassSpecification
	{
		Ref<PipelineCompute> Pipeline;
		std::string DebugName;
		glm::vec4 MarkerColor;
		uint32_t moreDescriptors = 0;
	};
	enum class InputType
	{
		sampler,
		stoage
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
		virtual void SetInputOneLayer(Ref<ImageView> imageView, uint32_t Binding, int index) = 0;
		virtual void SetInput(Ref<Texture2D> texture, uint32_t Binding, InputType type) = 0;
		virtual void SetInput(Ref<Image2D> texture, uint32_t Binding, uint32_t frameIndex) = 0;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type) = 0;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t frameIndex) = 0;  // Temp 解决Input到不同资源描述符，只能用于系统Init时的Pass
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t levelIndex, uint32_t descriptorIndex) = 0;  // Temp 解决Input到不同资源描述符，只能用于系统Init时的Pass
		
		
		// v2 版本

		// virtual void SetInput(std::string name, Ref<UniformBufferSet> ubSet) = 0;

		
		
		
		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};
}
