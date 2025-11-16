#pragma once
#include "RHIBase.h"
#include "RHIResource.h"
#include <GLFW/glfw3.h>
namespace Hazel {
	/*
	*  记录各种抽象API，这里的API是为了下层实现，上层并不是直接调用这里的API，而是需要走RHICommandList的封装，最终走到这里去找对应平台的实现
	*/

	// 上下文无关API
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









    protected:
		DynamicRHI(const RHIConfig& config) : m_Config(config) {};
		void RegisterResource(RHIResourceRef resource) { resourceMap[resource->GetType()].push_back(resource); } 

		std::array<std::vector<RHIResourceRef>, RHI_RESOURCE_TYPE_MAX_CNT> resourceMap;
        RHIConfig m_Config;
	};

	// 上下文相关的API
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