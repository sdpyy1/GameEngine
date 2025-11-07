#pragma once
#include <Hazel/Renderer/ComputePass.h>
#include <unordered_map>
namespace Hazel{
	struct EnvTextures {
		Ref<TextureCube> m_EnvCubeMap;
		Ref<TextureCube> m_EnvPreFilterMap;
		Ref<TextureCube> m_EnvIrradianceMap;
		Ref<Texture2D> m_EnvEquirect;
		Ref<Texture2D> m_EnvLut;
	};
	class EnvPass
    { 
	public:
		void Init();
		EnvTextures compute(std::filesystem::path path, Ref<RenderCommandBuffer> m_CommandBuffer, bool isInit = false);


	private:
		
		std::unordered_map<std::filesystem::path, EnvTextures> Envs;

		Ref<ComputePass> m_EquirectangularPass;
		Ref<ComputePass> m_EnvironmentIrradiancePass;
		Ref<ComputePass> m_EnvironmentPreFilterPass;
    };
}

