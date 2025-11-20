#pragma once
#include "Hazel/Asset/Asset.h"

namespace GameEngine {
	using ResourceDescriptorInfo = void*;
	class RendererResource : public Asset
	{
	public:
		virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
	};

}
