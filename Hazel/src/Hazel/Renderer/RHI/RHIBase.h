#pragma once
#include "Hazel/Core/Base.h"
namespace Hazel {
#define MAX_QUEUE_CNT 2					//每个队列族的最大队列数目


	typedef std::shared_ptr<class DynamicRHI> DynamicRHIRef;
	typedef std::shared_ptr<class RHIQueue> RHIQueueRef;


	enum RHIResourceType : uint32_t
	{
		RHI_BUFFER = 0,
		RHI_TEXTURE,
		RHI_TEXTURE_VIEW,
		RHI_SAMPLER,
		RHI_SHADER,
		RHI_SHADER_BINDING_TABLE,
		RHI_TOP_LEVEL_ACCELERATION_STRUCTURE,
		RHI_BOTTOM_LEVEL_ACCELERATION_STRUCTURE,

		RHI_ROOT_SIGNATURE,
		RHI_DESCRIPTOR_SET,

		RHI_RENDER_PASS,
		RHI_GRAPHICS_PIPELINE,
		RHI_COMPUTE_PIPELINE,
		RHI_RAY_TRACING_PIPELINE,

		RHI_QUEUE,
		RHI_SURFACE,
		RHI_SWAPCHAIN,
		RHI_COMMAND_POOL,
		RHI_COMMAND_CONTEXT,
		RHI_COMMAND_CONTEXT_IMMEDIATE,
		RHI_FENCE,
		RHI_SEMAPHORE,

		RHI_RESOURCE_TYPE_MAX_CNT,	//
	};
	enum QueueType : uint32_t
	{
		QUEUE_TYPE_GRAPHICS,
		QUEUE_TYPE_COMPUTE,
		QUEUE_TYPE_TRANSFER,

		QUEUE_TYPE_MAX_ENUM,
	};


	typedef struct RHIQueueInfo
	{
		QueueType type;
		uint32_t index;

	} RHIQueueInfo;



}