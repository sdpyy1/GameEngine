#pragma once

namespace Hazel {

	class RenderContext: public RefCounted
	{
	public:
		virtual ~RenderContext() = default;

		virtual void Init() = 0;  // ≥ı ºªØ‰÷»æAPI

		static Scope<RenderContext> Create_old(void* window);
	};

}
