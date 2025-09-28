#pragma once

namespace Hazel {

	class RenderContext
	{
	public:
		virtual ~RenderContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		static Scope<RenderContext> Create(void* window);
	};

}
