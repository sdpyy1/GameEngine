#pragma once

#include "Hazel/Core/Base.h"

#include "Framebuffer.h"

#include "UniformBufferSet.h"
#include "StorageBufferSet.h"
#include "Texture.h"
#include "Pipeline.h"
#include "StorageBuffer.h"

namespace Hazel {

	struct RenderPassSpecification
	{
		Ref<Pipeline> Pipeline;  // ???: 为什么要把RenderPass和Pipeline绑定起来（解答：这个render Pass不是Vulkan概念，这里的Pass封装了Pass需要的所有东西）
		std::string DebugName;
		glm::vec4 MarkerColor;
	};
	// 很宽泛的指一个渲染的Pass，而不是说Vulkan的RenderPass概念
	class RenderPass : public RefCounted
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassSpecification& GetSpecification() = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;
		virtual void SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding) = 0;
		virtual void SetInput(Ref<Texture2D> texture, uint32_t Binding) = 0;
		virtual void SetInput(Ref<Image2D> texture, uint32_t Binding, bool isInit = false) = 0;
		virtual Ref<Image2D> GetDepthOutput() = 0;
		virtual Ref<Pipeline> GetPipeline() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};

}
