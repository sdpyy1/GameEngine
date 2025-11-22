#include "hzpch.h"
#include "ImGuiPass.h"
#include "Hazel/Core/Application.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <Hazel/Editor/ImGuiRendererManager.h>
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"

namespace GameEngine
{
    void ImGuiPass::Init()
    {
        APP_DYNAMICRHI->InitImGui(APP_GLFWWINDOW);

        m_ImGuiRendererManager = std::make_shared<ImGuiRendererManager>();

    }
    void ImGuiPass::Build(RDGBuilder& builder)
	{
        if (IsEnabled())
        {
            RDGTextureHandle outColor = builder.GetTexture("RDG_TEXTURE_GRID");
            RDGTextureHandle depth = builder.GetTexture("RDG_TEXTURE_GRID_DEPTH");

            RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
                .Color(0, outColor, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
                .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)   // 为什么必须加深度才有效？
                .Execute([&](RDGPassContext context) {
                        auto [w, h] = APP_WINDOWSIZE;

                        Extent2D windowExtent = { w, h };

                        RHICommandListRef command = context.command;
                        command->SetViewport({ 0, 0 }, { windowExtent.width, windowExtent.height });
                        command->SetScissor({ 0, 0 }, { windowExtent.width, windowExtent.height });

                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        m_ImGuiRendererManager->ImGuiCommand();
                        ImGui::Render();
                        command->ImGuiRenderDrawData();
                    })
                .OutputReadWrite(outColor)
                .Finish();
        }
	}

}