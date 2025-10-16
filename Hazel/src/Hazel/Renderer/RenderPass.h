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
		Ref<Pipeline> Pipeline;  // ???: ΪʲôҪ��RenderPass��Pipeline��������������render Pass����Vulkan��������Pass��װ��Pass��Ҫ�����ж�����
		std::string DebugName;
		glm::vec4 MarkerColor;
	};
	// �ܿ���ָһ����Ⱦ��Pass��������˵Vulkan��RenderPass����
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
