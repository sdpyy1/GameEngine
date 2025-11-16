#pragma once
#include "RHIBase.h"
namespace Hazel {
	enum API {
		Vulkan,
        OpenGL,
        DirectX,
        None
	};
	struct RHIConfig {
		API api = None;
        bool debug = false;
		bool enableRayTracing = false;
	};
	class DynamicRHI {
	private:
		static DynamicRHIRef s_DynamicRHI;
	public:
        static DynamicRHIRef Init(RHIConfig config);
		static DynamicRHIRef Get(){return s_DynamicRHI;}
    protected:
		DynamicRHI(const RHIConfig& config) : m_Config(config) {};
        RHIConfig m_Config;
	};
}