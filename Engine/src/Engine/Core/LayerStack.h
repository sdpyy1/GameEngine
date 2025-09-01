#pragma once

#include "Engine/Core/Core.h"
#include "Layer.h"

#include <vector>

namespace Engine {

	class ENGINE_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);   // 覆盖层，直接栈顶
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
	private:
		std::vector<Layer*> m_Layers;
		// 维护普通层的插入位置，保证Overalay永远在最上边
		unsigned int m_LayerInsertIndex = 0;
	};

}
