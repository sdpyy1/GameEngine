#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"
#include "Hazel/Renderer/RenderPass/RenderPass.h"
#include "Hazel/Core/Definations.h"
namespace GameEngine
{
	class RenderSystem
	{
	public:
		RenderSystem();
		void InitPasses();
		void InitBaseResources();
		void Tick(float timestep);
		RHISwapchainRef GetSwapChain() { return m_SwapChain; }
		DynamicRHIRef GetRHI() { return m_DynamicRHI; }

	private:

		DynamicRHIRef m_DynamicRHI;
		RHISurfaceRef m_Surface;
		RHIQueueRef m_GraphicsQueue;
		RHISwapchainRef m_SwapChain;
		RHICommandPoolRef m_CommandPool;

		struct PerFrameBaseResource
		{
			RHICommandListRef commandList;
			RHISemaphoreRef startSemaphore;
			RHISemaphoreRef finishSemaphore;
			RHIFenceRef fence;
		};
		std::array<PerFrameBaseResource, FRAMES_IN_FLIGHT> m_PerFrameBaseResources;
		std::array<std::shared_ptr<RenderPassNew>, PASS_TYPE_MAX_CNT> passes;

	};




}

