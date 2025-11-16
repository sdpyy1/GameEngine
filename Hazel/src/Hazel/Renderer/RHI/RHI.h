#pragma once
#include "RHIBase.h"
#include "RHIResource.h"
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
	private:
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