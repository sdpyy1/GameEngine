#pragma once
#include "Hazel/Asset/Asset.h"

namespace Hazel {
	// 描述信息用来把某个句柄绑到这个void*上
	using ResourceDescriptorInfo = void*;
	// 表示这是一个渲染资源
	class RendererResource : public Asset
	{
	public:
		virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
	};

}
