#pragma once
#include "Hazel/Asset/Asset.h"

namespace Hazel {
	// ������Ϣ������ĳ����������void*��
	using ResourceDescriptorInfo = void*;
	// ��ʾ����һ����Ⱦ��Դ
	class RendererResource : public Asset
	{
	public:
		virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
	};

}
