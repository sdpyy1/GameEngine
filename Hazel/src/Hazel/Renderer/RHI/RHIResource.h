#pragma once
#include "RHIBase.h"
#include <queue>
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

		friend class DynamicRHI;
	};

	// ------------------------------------------------------------------------BASE ------------------------------------------------------------------------
	class RHIQueue : public RHIResource
	{
	public:
		RHIQueue(const RHIQueueInfo& info): RHIResource(RHI_QUEUE), info(info){}
		virtual void WaitIdle() = 0;

	protected:
		RHIQueueInfo info;
	};

	class RHISurface : public RHIResource
	{
	public:
		RHISurface() : RHIResource(RHI_SURFACE) {};

		inline Extent2D GetExetent() const { return extent; }

	protected:
		Extent2D extent;
	};

	class RHISwapchain : public RHIResource
	{
	public:
		RHISwapchain(const RHISwapchainInfo& info): RHIResource(RHI_SWAPCHAIN), info(info){}

		virtual uint32_t GetCurrentFrameIndex() = 0;
		virtual RHITextureRef GetTexture(uint32_t index) = 0;
		virtual RHITextureRef GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore) = 0;
		virtual void Present(RHISemaphoreRef waitSemaphore) = 0;

	protected:
		RHISwapchainInfo info;
	};



	class RHICommandPool : public RHIResource, public std::enable_shared_from_this<RHICommandPool>
	{
	public:
		RHICommandPool(const RHICommandPoolInfo& info): RHIResource(RHI_COMMAND_POOL), info(info){}

		RHICommandListRef CreateCommandList(bool byPass = true);

	protected:
		RHICommandPoolInfo info;

		std::queue<RHICommandContextRef> idleContexts = {};  // 暂未运行的线程
		std::vector<RHICommandContextRef> contexts = {};     // 所有分配的线程
		// CriticalSectionRef sync;

		void ReturnToPool(RHICommandContextRef commandContext) { idleContexts.push(commandContext); }
		friend class RHICommandList;
	};


	// ------------------------------------------------------------------------TEXTURE ------------------------------------------------------------------------
	class RHITexture : public RHIResource
	{
	public:
		RHITexture(const RHITextureInfo& info): RHIResource(RHI_TEXTURE), info(info){}

		Extent3D MipExtent(uint32_t mipLevel);

		inline const TextureSubresourceRange& GetDefaultSubresourceRange() const { return defaultRange; }
		inline const TextureSubresourceLayers& GetDefaultSubresourceLayers() const { return defaultLayers; }

		inline const RHITextureInfo& GetInfo() const { return info; }

	protected:
		RHITextureInfo info;

		TextureSubresourceRange defaultRange = {};
		TextureSubresourceLayers defaultLayers = {};
	};

	// ------------------------------------------------------------------------ 同步 ------------------------------------------------------------------------
	class RHIFence : public RHIResource
	{
	public:
		RHIFence(): RHIResource(RHI_FENCE){}

		virtual void Wait() = 0;
	};

	class RHISemaphore : public RHIResource
	{
	public:
		RHISemaphore(): RHIResource(RHI_SEMAPHORE){}
	};
}
