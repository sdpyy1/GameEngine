#include "hzpch.h"
#include "RenderSystem.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderPass/GridPass.h"
namespace GameEngine {
	RenderSystem::RenderSystem()
	{
        InitBaseResources();
	}

	void RenderSystem::InitBaseResources()
	{
		m_DynamicRHI = DynamicRHI::Init({ API_Vulkan,true,true });
		m_Surface = m_DynamicRHI->CreateSurface(APP_GLFWWINDOW);
		m_GraphicsQueue = m_DynamicRHI->GetQueue({ QUEUE_TYPE_GRAPHICS, 0 });
		m_SwapChain = m_DynamicRHI->CreateSwapChain({ m_Surface, m_GraphicsQueue, FRAMES_IN_FLIGHT, m_Surface->GetExetent(), SWAPCHAIN_COLOR_FORMAT });
		m_CommandPool = m_DynamicRHI->CreateCommandPool({ m_GraphicsQueue });
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++){
			m_PerFrameBaseResources[i].commandList = m_CommandPool->CreateCommandList(false);
			m_PerFrameBaseResources[i].startSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].finishSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].fence = m_DynamicRHI->CreateFence(true);
		}
	}

	void RenderSystem::Tick(float timestep)
	{
		auto& CurResource = m_PerFrameBaseResources[APP_FRAMEINDEX];

		CurResource.fence->Wait();
		RHITextureRef CurSwapchainTexture = m_SwapChain->GetNewFrame(nullptr, CurResource.startSemaphore);
		RHICommandListRef CurCommandList = CurResource.commandList;
		CurCommandList->BeginCommand();








		CurCommandList->EndCommand();
		CurCommandList->Execute(CurResource.fence, CurResource.startSemaphore, CurResource.finishSemaphore);
		m_SwapChain->Present(CurResource.finishSemaphore);
	}

	void RenderSystem::InitPass()
	{
		GridPass a;
		a.Init();
	}

}
