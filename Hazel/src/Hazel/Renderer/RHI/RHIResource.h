#pragma once
#include "RHIBase.h"
namespace Hazel {
	class RHIResource
	{
	public:
		RHIResource() = delete;
		RHIResource(RHIResourceType resourceType) : resourceType(resourceType) {};
		virtual ~RHIResource() {};

		inline RHIResourceType GetType() { return resourceType; }

		virtual void* RawHandle() { return nullptr; };		// 底层资源的裸指针，仅debug时使用

	private:
		RHIResourceType resourceType;
		uint32_t lastUseTick = 0;

		virtual void Destroy() {};

		friend class RHIBackend;
	};


	class RHIQueue : public RHIResource
	{
	public:
		RHIQueue(const RHIQueueInfo& info): RHIResource(RHI_QUEUE), info(info){}
		virtual void WaitIdle() = 0;

	protected:
		RHIQueueInfo info;
	};












}
