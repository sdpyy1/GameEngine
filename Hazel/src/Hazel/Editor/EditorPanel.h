#pragma once
#include <Hazel/Events/Event.h>
namespace Hazel {

	class EditorPanel : public RefCounted
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnEvent(Event& e) {}
		bool isOpen = true;
	};

}
