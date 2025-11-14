#pragma once
#include "EditorPanel.h"
#include "Hazel/Core/Log.h"

namespace Hazel {

	class ImGuiLogPanel : public EditorPanel
	{
	public:
		ImGuiLogPanel();
		virtual ~ImGuiLogPanel() = default;

		virtual void OnImGuiRender() override;

	private:
		bool m_ShowTrace;
		bool m_ShowInfo;
		bool m_ShowWarn;
		bool m_ShowError;
		bool m_ScrollToBottom;

		bool ShouldShow(Hazel::LogLevel level) const;
	};

} // namespace Hazel
