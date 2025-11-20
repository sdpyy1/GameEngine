#include "hzpch.h"
#include "ImGuiPass.h"
#include "Hazel/Core/Application.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
namespace GameEngine
{
    void ImGuiPass::Init()
    {
    }
    void ImGuiPass::Build(RDGBuilder& builder)
	{
        if (IsEnabled())
        {
            RDGTextureHandle outColor = builder.GetTexture("Bloom Out Color");
           //  RDGTextureHandle depth = builder.GetTexture("Depth");

            RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
                .Color(0, outColor, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
                // .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)   // 为什么必须加深度才有效？
                .Execute([&](RDGPassContext context) {
                        auto [w, h] = APP_WINDOWSIZE;

                        Extent2D windowExtent = { w, h };

                        RHICommandListRef command = context.command;
                        command->SetViewport({ 0, 0 }, { windowExtent.width, windowExtent.height });
                        command->SetScissor({ 0, 0 }, { windowExtent.width, windowExtent.height });

                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        ImGui::Begin("Debug Texture");
                        ImGui::Text("Bloom Out Color");
                        ImGui::End();
                        ImGui::Render();
                        command->ImGuiRenderDrawData();
                    })
                .Finish();
        }
	}

}