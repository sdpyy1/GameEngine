#include "hzpch.h"
#include "PresentPass.h"
#include "Hazel/Core/Application.h"
namespace GameEngine
{ 
	void PresentPass::Init()
	{
	}

	void PresentPass::Build(RDGBuilder& builder)
	{
        RHITextureRef swapchainTexture = APP_SWAPCHAIN->GetTexture(APP_FRAMEINDEX);

        RDGTextureHandle outColor = builder.GetTexture("Bloom Out Color");

        RDGTextureHandle present = builder.CreateTexture("Present")
            .Import(swapchainTexture, RESOURCE_STATE_PRESENT)
            .Finish();

        RDGPresentPassHandle pass = builder.CreatePresentPass(GetName())
            .PresentTexture(present)
            .Texture(outColor)
            .Finish();
    }
}