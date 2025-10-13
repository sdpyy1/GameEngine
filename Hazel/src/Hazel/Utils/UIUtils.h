#pragma once
#include <Platform/Vulkan/VulkanImage.h>
#include <examples/imgui_impl_vulkan_with_textures.h>
namespace Hazel {
	namespace UI {
		static void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col = { 1,1,1,1 }, const ImVec4& border_col = { 0,0,0,0 })
		{
			HZ_CORE_VERIFY(image, "Image is null");
			if (RendererAPI::Current() == RendererAPI::Type::Vulkan)
			{
				Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
				const auto& imageInfo = vulkanImage->GetImageInfo();
				if (!imageInfo.ImageView)
					return;
				const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
				ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
			}
		}
	}
}
