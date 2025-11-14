#include "hzpch.h"
#include "ImGuiRendererManager.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Platform/Vulkan/VulkanImGuiLayer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <imgui.h>
namespace Hazel {
	namespace Colors
	{
		// To experiment with editor theme live you can change these constexpr into static
		// members of a static "Theme" class and add a quick ImGui window to adjust the colour values
		namespace Theme
		{
			constexpr auto accent = IM_COL32(236, 158, 36, 255);
			constexpr auto highlight = IM_COL32(39, 185, 242, 255);
			constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
			constexpr auto compliment = IM_COL32(78, 151, 166, 255);
			constexpr auto background = IM_COL32(36, 36, 36, 255);
			constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
			constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
			constexpr auto titlebarOrange = IM_COL32(186, 66, 30, 255);
			constexpr auto titlebarGreen = IM_COL32(18, 88, 30, 255);
			constexpr auto titlebarRed = IM_COL32(185, 30, 30, 255);
			constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
			constexpr auto text = IM_COL32(192, 192, 192, 255);
			constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
			constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
			constexpr auto textError = IM_COL32(230, 51, 51, 255);
			constexpr auto muted = IM_COL32(77, 77, 77, 255);
			constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
			constexpr auto selection = IM_COL32(237, 192, 119, 255);
			constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);
			constexpr auto backgroundPopup = IM_COL32(50, 50, 50, 255);
			constexpr auto validPrefab = IM_COL32(82, 179, 222, 255);
			constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);
			constexpr auto missingMesh = IM_COL32(230, 102, 76, 255);
			constexpr auto meshNotSet = IM_COL32(250, 101, 23, 255);
		}
	}
	std::shared_ptr<ImGuiRendererManager> ImGuiRendererManager::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::Type::None:    return nullptr;
			case RendererAPI::Type::Vulkan:  return std::make_shared<VulkanImGuiLayer>();
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}


	void ImGuiRendererManager::SetDarkThemeV2Colors()
	{
		auto& style = ImGui::GetStyle();
		auto& colors = ImGui::GetStyle().Colors;

		//========================================================
		/// Colours

		// Headers
		colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);

		// Buttons
		colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);

		// Tabs
		colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
		colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
		colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		// Title
		colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Checkbox
		colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Separator
		colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);
		colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::highlight);
		colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

		// Window Background
		colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::background);
		colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundPopup);
		colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

		//========================================================
		/// Style
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 11.0f;
	}

}
