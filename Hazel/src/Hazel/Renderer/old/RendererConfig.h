#pragma once
#include <string>

namespace GameEngine {
	// ‰÷»æ≈‰÷√£¨¥Ê¥¢‘⁄Renderer¿‡÷–
	struct RendererConfig
	{
		uint32_t FramesInFlight = 3;

		bool ComputeEnvironmentMaps = true;

		// Tiering settings
		uint32_t EnvironmentMapResolution = 1024;
		uint32_t IrradianceMapComputeSamples = 512;
		uint32_t IrradianceMapSize = 32;
	};
}
