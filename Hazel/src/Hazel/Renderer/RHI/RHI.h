#pragma once
#include "RHIBase.h"
#include "RHIResource.h"
#include <GLFW/glfw3.h>
namespace Hazel {
	/* RHI总览
	* 上下文无关的操作，定义在DynamicRHI的虚函数，Init时进行各个API的初始化操作
	* 上下文相关的操作，定义在RHICommandContext的虚函数，创建RHICommandContext对象时，底层会创建一个上下文handle，比如Vulkan会创建一个VkCommandBuffer
	* 需要立即执行的命令，比如转换图片布局，定义在RHICommandContextImmediate，它需要上下文，但是需要Flush立即执行，底层也会创建一个上下文并且需要Fence控制并发
	*/

	class DynamicRHI {
	private:
		static DynamicRHIRef s_DynamicRHI;
	public:
        static DynamicRHIRef Init(RHIConfig config);
		static DynamicRHIRef Get(){return s_DynamicRHI;}

		virtual void Tick();    // 更新资源计数，清理无引用且长时间未使用资源

		virtual void Destroy();

		virtual RHIQueueRef GetQueue(const RHIQueueInfo& info) = 0;

		virtual RHISurfaceRef CreateSurface(GLFWwindow* window) = 0;

		virtual RHISwapchainRef CreateSwapChain(const RHISwapchainInfo& info) = 0;

		virtual RHICommandPoolRef CreateCommandPool(const RHICommandPoolInfo& info) = 0;

		virtual RHICommandContextRef CreateCommandContext(RHICommandPoolRef pool) = 0;

		virtual RHIFenceRef CreateFence(bool signaled) = 0;

		virtual RHISemaphoreRef CreateSemaphore() = 0;

    protected:
		DynamicRHI(const RHIConfig& config) : m_Config(config) {};
		void RegisterResource(RHIResourceRef resource) { resourceMap[resource->GetType()].push_back(resource); } 

		std::array<std::vector<RHIResourceRef>, RHI_RESOURCE_TYPE_MAX_CNT> resourceMap;
        RHIConfig m_Config;
	};



	class RHICommandContext : public RHIResource {
	public:
		RHICommandContext(RHICommandPoolRef pool): RHIResource(RHI_COMMAND_CONTEXT), pool(pool){}
		virtual void BeginCommand() = 0;
		virtual void EndCommand() = 0;
	protected:
		RHICommandPoolRef pool;
	};



	class RHICommandContextImmediate : public RHIResource
	{
	public:
		RHICommandContextImmediate(): RHIResource(RHI_COMMAND_CONTEXT_IMMEDIATE){}

		virtual void Flush() = 0;
		virtual void GenerateMips(RHITextureRef src) = 0;

	};













	
}